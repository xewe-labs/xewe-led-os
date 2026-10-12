// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeKit/HomeKit.cpp

#include "HomeKit.h"

#include "HomeKitMap.h"

HomeKit* HomeKit::instance = nullptr;

HomeKit::HomeKit(xewe::Os& host, Wifi& wifi, Led& led)
    : xewe::Module(host,
          /* id                  */ "homekit",
          /* name                */ "HomeKit",
          /* description         */
          "Allows to control the LED via Apple Home App.\nREQUIRES Apple Hub (Speaker/Apple TV)",
          /* requires_init_setup */ true,
          /* can_be_disabled     */ true,
          /* has_cli_cmds        */ true),
      led(led) {
    add_requirement(wifi);
}

HomeKit::NeoPixel_RGB::NeoPixel_RGB(HomeKit* owner_module)
    : Service::LightBulb(), owner(owner_module) {
    V.setRange(1, 100, 1);
}

// Home app -> led. Colour, brightness and state are applied on every write, whichever of them
// changed; the colour is sent at full value, the brightness separately.
boolean HomeKit::NeoPixel_RGB::update() {
    if (!owner)
        return false;

    const bool state = power.getNewVal();
    const float hue_deg = H.getNewVal<float>(); // 0..360
    const float sat_pct = S.getNewVal<float>(); // 0..100
    const float bri_pct = V.getNewVal<float>(); // 1..100

    const uint8_t h_byte = led_map::deg_to_hue(hue_deg);
    const uint8_t s_byte = led_map::pct_to_byte(sat_pct);
    const uint8_t bri_byte = led_map::pct_to_byte(bri_pct);

    const std::array<uint8_t, 3> rgb = xewe::color::hsv_to_rgb({h_byte, s_byte, 255});
    owner->led.sync_color(rgb, owner);
    owner->led.sync_brightness(bri_byte, owner);
    owner->led.sync_state(state, owner);

    return true;
}

// led -> Home app. setVal() does not call update(), so nothing comes back to the led.
void HomeKit::on_color(std::array<uint8_t, 3> rgb, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;

    const std::array<uint8_t, 3> hsv = xewe::color::rgb_to_hsv(rgb);
    const float hue_deg = static_cast<float>(led_map::hue_to_deg(hsv[0]));
    const float sat_pct = static_cast<float>(led_map::sat_to_pct(hsv[1]));

    device->H.setVal(hue_deg);
    device->S.setVal(sat_pct);
}

// 0 is passed through although HomeKit's range starts at 1 (HomeSpan stores it).
void HomeKit::on_brightness(uint8_t brightness, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;

    const float bri_pct = static_cast<float>(led_map::bri_to_pct(brightness));
    device->V.setVal(bri_pct);
}

void HomeKit::on_state(bool on, const void* origin) {
    if (origin == this)
        return;
    if (is_disabled())
        return;
    if (!device)
        return;

    device->power.setVal(on);
}

// HomeKit has no modes
void HomeKit::on_mode(uint8_t mode_id, const void* origin) {
    (void)mode_id;
    (void)origin;
}

void HomeKit::on_all(std::array<uint8_t, 3> rgb, uint8_t brightness, bool on, uint8_t mode_id,
    uint16_t length, const void* origin) {
    (void)mode_id;
    (void)length;
    if (is_disabled())
        return;
    if (!device)
        return;

    on_state(on, origin);
    on_brightness(brightness, origin);
    on_color(rgb, origin);
}

void HomeKit::begin_routines_required() {
    instance = this;

    const std::string device_name = os.system.get_device_name();

    homeSpan.setPortNum(1201);
    homeSpan.setSerialInputDisable(true);
    homeSpan.setLogLevel(-1);
    homeSpan.begin(Category::Lighting, device_name.c_str());

    SPAN_ACCESSORY();
    SPAN_ACCESSORY(device_name.c_str());

    device = new NeoPixel_RGB(this);

    // registered here, not in the constructor, so a disabled module gets no led events
    if (!led.listeners.add(this))
        os.report_error(
            "! %s: led listener list full, Home app will not follow led changes", name.c_str());
}

void HomeKit::begin_routines_init() {
    homeSpan.setStatusCallback(&HomeKit::status_callback);
    os.serial.print("\nOpen the link with the Setup QR below and scan it\nwith your iPhone/iPad");
    os.serial.print(HOMEKIT_QR_URL);
    os.serial.print("If using Mac, go to the Home App and add device\nusing code 4663-7726");
    os.serial.print(
        "\nThe setup process will continue automatically\nafter device is pared with HomeKit");

    os.serial.print("TO ABORT PRESS (x): ");
    while (hs_status != HS_PAIRED) {
        homeSpan.poll();
        os.serial.loop();
        if (os.serial.has_line()) {
            std::string input = os.serial.read_line();
            if (input[0] == 'x') {
                disable(false, true); // reset with no verbose and restart
                return;
            } else
                os.serial.print("\n(x)?: ");
        }
    }

    os.serial.print("Setting up HomeKit");
    run_with_dots([] { homeSpan.poll(); }, 3000);
    os.serial.print("\nDevice successfully paired with HomeKit.\nNote, it will stop working with "
                    "HomeKit App if you dont have a hub");
}

void HomeKit::loop() {
    if (is_disabled())
        return;
    homeSpan.poll();
}

// Erases the pairing list only ('U'); the Wi-Fi credentials HomeSpan keeps and the stored
// characteristic values stay. The Home app still lists the device until it is removed there.
void HomeKit::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    if (verbose)
        os.serial.print("You also need to remove the device from the Home App manually");
    // HomeSpan's 'U' needs homeSpan.begin() (it updates the status LED); before that only the
    // stored pairing list is erased.
    if (device)
        homeSpan.processSerialCommand("U");
    else
        os.nvs.remove("HAP", "CONTROLLERS");
    delay(100);
    xewe::Module::reset(verbose, do_restart, keep_enabled);
}

// The HomeSpan dumps go straight to the serial port, before the module line. `$system status`
// asks every module for its status, so it prints them too.
std::string HomeKit::status(const bool verbose) const {
    if (is_disabled())
        return std::string("HomeKit module disabled");
    homeSpan.setLogLevel(2);
    homeSpan.processSerialCommand("s");
    homeSpan.processSerialCommand("i");
    homeSpan.setLogLevel(-1);
    return xewe::Module::status(verbose);
}

void HomeKit::status_callback(HS_STATUS s) {
    if (instance) {
        instance->hs_status = static_cast<uint8_t>(s);
    }
}
