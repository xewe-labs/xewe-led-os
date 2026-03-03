/*********************************************************************************
 * SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 * Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 * See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 * Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 * https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/LedStrip.cpp

#include "LedStrip.h"

#include "../../../SystemController/SystemController.h"

// =============================================================================
// Constructor
// =============================================================================
LedStrip::LedStrip(SystemController& controller)
      : Interface(controller,
               /* interface_name        */  "Led",
               /* interface_description */  "Allows to control addressable LED strip",
               /* nvs_key               */  "led",
               /* requires_init_setup   */  true,
               /* can_be_disabled       */  false,
               /* has_cli_cmds          */  true)
{
    DBG_PRINTLN(LedStrip, "-> LedStrip::LedStrip()");

    commands_storage.push_back({
        "set_rgb",
        "Set RGB color",
        string("$") + lower(module_name) + " set_rgb 255 0 0",
        3,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_rgb triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;
            int i2 = args.indexOf(' ', i1 + 1);
            if (i2 == -1) return;

            uint8_t r = args.substring(0, i1).toInt();
            uint8_t g = args.substring(i1 + 1, i2).toInt();
            uint8_t b = args.substring(i2 + 1).toInt();

            controller.sync_color({r, g, b}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_r",
        "Set red channel",
        string("$") + lower(module_name) + " set_r 127",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_r triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_color({(uint8_t)args.toInt(), get_g(), get_b()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_g",
        "Set green channel",
        string("$") + lower(module_name) + " set_g 255",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_g triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_color({get_r(), (uint8_t)args.toInt(), get_b()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_b",
        "Set blue channel",
        string("$") + lower(module_name) + " set_b 200",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_b triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_color({get_r(), get_g(), (uint8_t)args.toInt()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_hsv",
        "Set HSV color",
        string("$") + lower(module_name) + " set_hsv 75 255 0",
        3,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_hsv triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;
            int i2 = args.indexOf(' ', i1 + 1);
            if (i2 == -1) return;

            uint8_t h = args.substring(0, i1).toInt();
            uint8_t s = args.substring(i1 + 1, i2).toInt();
            uint8_t v = args.substring(i2 + 1).toInt();

            array<uint8_t, 3> new_rgb = ModeController::hsv_to_rgb({h, s, v});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_hue",
        "Set hue channel",
        string("$") + lower(module_name) + " set_hue 255",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_hue triggered");
            String args(args_sv.data(), args_sv.length());
            array<uint8_t, 3> current_hsv = get_hsv();
            array<uint8_t, 3> new_rgb = ModeController::hsv_to_rgb({(uint8_t)args.toInt(), current_hsv[1], current_hsv[2]});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_sat",
        "Set saturation channel",
        string("$") + lower(module_name) + " set_sat 0",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_sat triggered");
            String args(args_sv.data(), args_sv.length());
            array<uint8_t, 3> current_hsv = get_hsv();
            array<uint8_t, 3> new_rgb = ModeController::hsv_to_rgb({current_hsv[0], (uint8_t)args.toInt(), current_hsv[2]});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_val",
        "Set value channel",
        string("$") + lower(module_name) + " set_val 255",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_val triggered");
            String args(args_sv.data(), args_sv.length());
            array<uint8_t, 3> current_hsv = get_hsv();
            array<uint8_t, 3> new_rgb = ModeController::hsv_to_rgb({current_hsv[0], current_hsv[1], (uint8_t)args.toInt()});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_brightness",
        "Set global brightness",
        string("$") + lower(module_name) + " set_brightness 255",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_brightness triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_brightness(args.toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_state",
        "Set on/off state",
        string("$") + lower(module_name) + " set_state 0",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_state triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_state(args.toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "toggle_state",
        "If off->on, if on->off",
        string("$") + lower(module_name) + " toggle_state",
        0,
        [this, &controller](string_view) {
            DBG_PRINTLN(LedStrip, "CMD: toggle_state triggered");
            controller.sync_state(!get_state(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "turn_on",
        "Turn strip on",
        string("$") + lower(module_name) + " turn_on",
        0,
        [this, &controller](string_view) {
            DBG_PRINTLN(LedStrip, "CMD: turn_on triggered");
            controller.sync_state(1, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "turn_off",
        "Turn strip off",
        string("$") + lower(module_name) + " turn_off",
        0,
        [this, &controller](string_view) {
            DBG_PRINTLN(LedStrip, "CMD: turn_off triggered");
            controller.sync_state(0, {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_mode",
        "Set LED strip mode",
        string("$") + lower(module_name) + " set_mode 0",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_mode triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_mode(args.toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_length",
        "Set new number of LEDs",
        string("$") + lower(module_name) + " set_length 500",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_length triggered");
            String args(args_sv.data(), args_sv.length());
            controller.sync_length(args.toInt(), {true, true, true, true, true});
        }
    });

    DBG_PRINTLN(LedStrip, "<- LedStrip::LedStrip()");
}

// =============================================================================
// Interface Sync
// =============================================================================
void LedStrip::sync_color(array<uint8_t,3> color) {
    DBG_PRINTF(LedStrip, "-> sync_color(%u, %u, %u)\n", color[0], color[1], color[2]);
    set_rgb(color);
    DBG_PRINTLN(LedStrip, "<- sync_color()");
}

void LedStrip::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(LedStrip, "-> sync_brightness(%u)\n", brightness);
    set_brightness(brightness);
    DBG_PRINTLN(LedStrip, "<- sync_brightness()");
}

void LedStrip::sync_state(uint8_t state) {
    DBG_PRINTF(LedStrip, "-> sync_state(%u)\n", state);
    set_state(state);
    DBG_PRINTLN(LedStrip, "<- sync_state()");
}

void LedStrip::sync_mode(uint8_t mode) {
    DBG_PRINTF(LedStrip, "-> sync_mode(%u)\n", mode);
    set_mode(mode);
    DBG_PRINTLN(LedStrip, "<- sync_mode()");
}

void LedStrip::sync_length(uint16_t length) {
    DBG_PRINTF(LedStrip, "-> sync_length(%u)\n", length);
    set_length(length);
    DBG_PRINTLN(LedStrip, "<- sync_length()");
}

// =============================================================================
// Module Logic
// =============================================================================
void LedStrip::begin_routines_required(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_required()");
    const auto& config = static_cast<const LedStripConfig&>(cfg);
    this->num_led                = config.num_led;
    this->mode_transition_delay  = config.mode_transition_delay;

    DBG_PRINTF(LedStrip, "Config Num LEDs: %u, Transition Delay: %u\n", num_led, mode_transition_delay);

    FastLED.addLeds<LED_STRIP_TYPE, PIN_LED_STRIP, LED_STRIP_COLOR_ORDER>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(255);

    frame_timer = make_unique<AsyncTimer<uint8_t>>(config.frame_delay);
    brightness = make_unique<Brightness>(config.brightness_transition_delay, 0, 0);
    mode_controller = make_unique<ModeController>(*this, mode_transition_delay);

    frame_timer->initiate();
    DBG_PRINTLN(LedStrip, "<- begin_routines_required()");
}

void LedStrip::begin_routines_init(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_init()");
    this->num_led = controller.serial_port.get_int("How many LEDs do you have connected", 0, LED_STRIP_NUM_LEDS_MAX + 1);
    DBG_PRINTF(LedStrip, "User input Num LEDs: %u\n", this->num_led);

    controller.sync_all(
        {0, 255,  0},
        50,
        1,
        0,
        this->num_led,
        {true, true, false, false, false} //only write to nvs and led
    );
    controller.serial_port.print("\nLED strip is set to green\n"
                                   "If you don't see the green color check the\n"
                                   "pin (GPIO), led type, and color order\n\n"
                                   "LED setup success!");
    DBG_PRINTLN(LedStrip, "<- begin_routines_init()");
}

void LedStrip::begin_routines_regular(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_regular()");
    controller.nvs.sync_from_memory({true, false, false, false, false});
    DBG_PRINTLN(LedStrip, "<- begin_routines_regular()");
}

void LedStrip::begin_routines_common(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_common()");
    controller.serial_port.print("Setting up LED lights");
    run_with_dots([this] { loop(); }, (float) mode_transition_delay * 1.2f);
    DBG_PRINTLN(LedStrip, "<- begin_routines_common()");
}

void LedStrip::loop() {
    // Note: No logging in loop() to prevent Serial spam and frame drops
    if (frame_timer->is_not_done()) return;
    frame_timer->reset();
    frame_timer->initiate();

    set_all(mode_controller->loop());
    fps_counter++;
}

void LedStrip::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    DBG_PRINTLN(LedStrip, "-> reset()");
    controller.sync_all(
        {0, 255,  0},
        50,
        1,
        0,
        this->num_led,
        {true, true, true, true, true}
    );
    if (verbose) status(true);
    Module::reset(verbose, do_restart, keep_enabled);
    DBG_PRINTLN(LedStrip, "<- reset()");
}

string LedStrip::status(const bool verbose) const {
    DBG_PRINTLN(LedStrip, "-> status()");
    std::stringstream status_stream;
    status_stream << "Hardware Setting:\n"
                  << "    Pin:          GPIO" << static_cast<int>(PIN_LED_STRIP) << "\n"
                  << "    Type:         " << TO_STRING(LED_STRIP_TYPE) << "\n"
                  << "    Color Order:  " << TO_STRING(LED_STRIP_COLOR_ORDER) << "\n"
                  << "    Max LEDs:     " << LED_STRIP_NUM_LEDS_MAX << "\n"
                  << "\n"
                  << "Live State:\n"
                  << "    FPS:          " << fps_counter * 1000 / millis()  << "\n"
                  << "    Length:       " << get_length() << "\n"
                  << "    State:        " << (get_state() ? "ON" : "OFF") << "\n"
                  << "    Brightness:   " << static_cast<int>(get_brightness()) << "\n"
                  << "    Mode:         " << get_mode_name().c_str() << "\n"
                  << "    Color (RGB):  ("
                  << static_cast<int>(get_r()) << ", "
                  << static_cast<int>(get_g()) << ", "
                  << static_cast<int>(get_b()) << ")";

    std::string status_string = status_stream.str();
    if (verbose) controller.serial_port.print(status_string.c_str());
    DBG_PRINTLN(LedStrip, "<- status()");
    return status_string;
}

// =============================================================================
// Custom Methods: Color
// =============================================================================
void LedStrip::set_rgb(const array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(LedStrip, "-> set_rgb(%u, %u, %u)\n", new_rgb[0], new_rgb[1], new_rgb[2]);
    mode_controller->set_rgb(new_rgb);
    DBG_PRINTLN(LedStrip, "<- set_rgb()");
}

void LedStrip::set_r(const uint8_t r) {
    DBG_PRINTF(LedStrip, "-> set_r(%u)\n", r);
    array<uint8_t, 3> old_rgb = get_rgb();
    mode_controller->set_rgb({r, old_rgb[1], old_rgb[2]});
    DBG_PRINTLN(LedStrip, "<- set_r()");
}

void LedStrip::set_g(const uint8_t g) {
    DBG_PRINTF(LedStrip, "-> set_g(%u)\n", g);
    array<uint8_t, 3> old_rgb = get_rgb();
    mode_controller->set_rgb({old_rgb[0], g, old_rgb[2]});
    DBG_PRINTLN(LedStrip, "<- set_g()");
}

void LedStrip::set_b(const uint8_t b) {
    DBG_PRINTF(LedStrip, "-> set_b(%u)\n", b);
    array<uint8_t, 3> old_rgb = get_rgb();
    mode_controller->set_rgb({old_rgb[0], old_rgb[1], b});
    DBG_PRINTLN(LedStrip, "<- set_b()");
}

void LedStrip::set_hsv(const array<uint8_t, 3> new_hsv) {
    DBG_PRINTF(LedStrip, "-> set_hsv(%u, %u, %u)\n", new_hsv[0], new_hsv[1], new_hsv[2]);
    array<uint8_t, 3> new_rgb = ModeController::hsv_to_rgb(new_hsv);
    mode_controller->set_rgb(new_rgb);
    DBG_PRINTLN(LedStrip, "<- set_hsv()");
}

void LedStrip::set_h(const uint8_t h) {
    DBG_PRINTF(LedStrip, "-> set_h(%u)\n", h);
    array<uint8_t, 3> old_hsv = get_hsv();
    mode_controller->set_rgb(ModeController::hsv_to_rgb({h, old_hsv[1], old_hsv[2]}));
    DBG_PRINTLN(LedStrip, "<- set_h()");
}

void LedStrip::set_s(const uint8_t s) {
    DBG_PRINTF(LedStrip, "-> set_s(%u)\n", s);
    array<uint8_t, 3> old_hsv = get_hsv();
    mode_controller->set_rgb(ModeController::hsv_to_rgb({old_hsv[0], s, old_hsv[2]}));
    DBG_PRINTLN(LedStrip, "<- set_s()");
}

void LedStrip::set_v(const uint8_t v) {
    DBG_PRINTF(LedStrip, "-> set_v(%u)\n", v);
    array<uint8_t, 3> old_hsv = get_hsv();
    mode_controller->set_rgb(ModeController::hsv_to_rgb({old_hsv[0], old_hsv[1], v}));
    DBG_PRINTLN(LedStrip, "<- set_v()");
}

array<uint8_t, 3> LedStrip::get_rgb() const {
    // DBG_PRINTLN(LedStrip, "-> get_rgb()");
    return mode_controller->get_rgb();
}

uint8_t LedStrip::get_r() const {
    // DBG_PRINTLN(LedStrip, "-> get_r()");
    return get_rgb()[0];
}

uint8_t LedStrip::get_g() const {
    // DBG_PRINTLN(LedStrip, "-> get_g()");
    return get_rgb()[1];
}

uint8_t LedStrip::get_b() const {
    // DBG_PRINTLN(LedStrip, "-> get_b()");
    return get_rgb()[2];
}

array<uint8_t, 3> LedStrip::get_hsv() const {
    // DBG_PRINTLN(LedStrip, "-> get_hsv()");
    array<uint8_t, 3> rgb = get_rgb();
    return ModeController::rgb_to_hsv(rgb);
}

uint8_t LedStrip::get_h() const {
    // DBG_PRINTLN(LedStrip, "-> get_h()");
    return get_hsv()[0];
}

uint8_t LedStrip::get_s() const {
    // DBG_PRINTLN(LedStrip, "-> get_s()");
    return get_hsv()[1];
}

uint8_t LedStrip::get_v() const {
    // DBG_PRINTLN(LedStrip, "-> get_v()");
    return get_hsv()[2];
}

// =============================================================================
// Custom Methods: Brightness
// =============================================================================
void LedStrip::set_brightness(const uint8_t new_brightness) {
    DBG_PRINTF(LedStrip, "-> set_brightness(%u)\n", new_brightness);
    brightness->set_brightness(new_brightness);
    DBG_PRINTLN(LedStrip, "<- set_brightness()");
}

uint8_t LedStrip::get_brightness() const {
    // DBG_PRINTLN(LedStrip, "-> get_brightness()");
    return brightness->get_last_brightness();
}

// =============================================================================
// Custom Methods: State
// =============================================================================
void LedStrip::set_state(const uint8_t state) {
    DBG_PRINTF(LedStrip, "-> set_state(%u)\n", state);
    if (state) {
        turn_on();
    } else {
        turn_off();
    }
    DBG_PRINTLN(LedStrip, "<- set_state()");
}

void LedStrip::toggle_state() {
    DBG_PRINTLN(LedStrip, "-> toggle_state()");
    if(get_state()) {
        turn_off();
    } else {
        turn_on();
    }
    DBG_PRINTLN(LedStrip, "<- toggle_state()");
}

void LedStrip::turn_on() {
    DBG_PRINTLN(LedStrip, "-> turn_on()");
    brightness->turn_on();
    DBG_PRINTLN(LedStrip, "<- turn_on()");
}

void LedStrip::turn_off() {
    DBG_PRINTLN(LedStrip, "-> turn_off()");
    brightness->turn_off();
    DBG_PRINTLN(LedStrip, "<- turn_off()");
}

bool LedStrip::get_state() const {
    // DBG_PRINTLN(LedStrip, "-> get_state()");
    return brightness->get_state();
}

// =============================================================================
// Custom Methods: Mode
// =============================================================================
void LedStrip::set_mode(const uint8_t new_mode) {
    DBG_PRINTF(LedStrip, "-> set_mode(%u)\n", new_mode);
    mode_controller->set_mode(new_mode);
    DBG_PRINTLN(LedStrip, "<- set_mode()");
}

uint8_t LedStrip::get_mode_id() const {
    // DBG_PRINTLN(LedStrip, "-> get_mode_id()");
    return mode_controller->get_mode_id();
}

string LedStrip::get_mode_name() const {
    // DBG_PRINTLN(LedStrip, "-> get_mode_name()");
    return mode_controller->get_mode_name();
}

string LedStrip::get_all_modes() const {
    DBG_PRINTLN(LedStrip, "-> get_all_modes()");
    return mode_controller->get_all_modes();
}

// =============================================================================
// Custom Methods: Length
// =============================================================================
void LedStrip::set_length(const uint16_t length) {
    DBG_PRINTF(LedStrip, "-> set_length(%u)\n", length);
    if (length > LED_STRIP_NUM_LEDS_MAX) {
        controller.serial_port.print("That's too many. Max supported: " + to_string(LED_STRIP_NUM_LEDS_MAX) + " LEDs");
        DBG_PRINTLN(LedStrip, "Error: set_length exceeded MAX limit");
        return;
    }

    set_black();
    num_led = length;
    DBG_PRINTLN(LedStrip, "<- set_length()");
}

uint16_t LedStrip::get_length() const {
    // DBG_PRINTLN(LedStrip, "-> get_length()");
    return num_led;
}

// =============================================================================
// Custom Methods: Fill
// =============================================================================

// Helper: Sets a single pixel with brightness correction
void LedStrip::set_pixel(uint16_t i, std::array<uint8_t, 3> color_rgb) {
    // NO LOGGING HERE: Called per pixel per frame. High frequency.
    if (i < num_led) {
        std::array<uint8_t, 3> dimmed_color = brightness->get_dimmed_color(color_rgb);
        leds[i] = CRGB(dimmed_color[0], dimmed_color[1], dimmed_color[2]);
    }
}

/**
 * @brief Sets the entire strip from a CRGB array (usually from ModeController).
 * Iterates through the input array and uses set_pixel to apply brightness/color corrections.
 */
void LedStrip::set_all(CRGB* new_leds) {
    // NO LOGGING HERE: Called every frame. High frequency.
    if (new_leds != nullptr) {
        for (uint16_t i = 0; i < num_led; i++) {
            // Convert CRGB to std::array for the helper
            set_pixel(i, {new_leds[i].r, new_leds[i].g, new_leds[i].b});
        }
        FastLED.show();
    }
}

/**
 * @brief Sets the entire strip to a specific solid RGB color.
 */
void LedStrip::set_all(const uint8_t r, const uint8_t g, const uint8_t b) {
    // DBG_PRINTF(LedStrip, "set_all solid (%u, %u, %u)\n", r, g, b); // Optional: enable if debugging solid fills
    for (uint16_t i = 0; i < num_led; i++) {
        set_pixel(i, {r, g, b});
    }
    FastLED.show();
}

/**
 * @brief Turns off all LEDs (sets to Black/0,0,0).
 */
void LedStrip::set_black() {
    DBG_PRINTLN(LedStrip, "-> set_black()");
    fill_solid(leds, num_led, CRGB::Black);
    FastLED.show();
    DBG_PRINTLN(LedStrip, "<- set_black()");
}