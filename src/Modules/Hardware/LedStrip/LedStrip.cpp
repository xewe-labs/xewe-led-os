// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Hardware/LedStrip/LedStrip.cpp

#include "LedStrip.h"
#include "../../Module/ModuleController.h"


// =============================================================================
// Constructor
// =============================================================================
LedStrip::LedStrip(ModuleController& controller)
    : SyncModule(controller,
          /* id                    */ "led",
          /* name                  */ "Led",
          /* description           */ "Allows to control addressable LED strip",
          /* requires_init_setup   */ true,
          /* can_be_disabled       */ false,
          /* has_cli_cmds          */ true
    )
{
    DBG_PRINTLN(LedStrip, "-> LedStrip::LedStrip()");

    commands_storage.push_back(Command{
        "set_rgb",
        "Set RGB color",
        std::string("$") + id + " set_rgb 255 0 0",
        3,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_rgb triggered");
            uint8_t r = String(args[0].c_str()).toInt();
            uint8_t g = String(args[1].c_str()).toInt();
            uint8_t b = String(args[2].c_str()).toInt();

            controller.sync_color({r, g, b}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_r",
        "Set red channel",
        std::string("$") + id + " set_r 127",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_r triggered");
            controller.sync_color({(uint8_t)String(args[0].c_str()).toInt(), get_g(), get_b()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_g",
        "Set green channel",
        std::string("$") + id + " set_g 255",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_g triggered");
            controller.sync_color({get_r(), (uint8_t)String(args[0].c_str()).toInt(), get_b()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_b",
        "Set blue channel",
        std::string("$") + id + " set_b 200",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_b triggered");
            controller.sync_color({get_r(), get_g(), (uint8_t)String(args[0].c_str()).toInt()}, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_rgb",
        "Adjust RGB color by deltas",
        std::string("$") + id + " adj_rgb 10 -20 5",
        3,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_rgb triggered");
            int r_d = String(args[0].c_str()).toInt();
            int g_d = String(args[1].c_str()).toInt();
            int b_d = String(args[2].c_str()).toInt();

            this->adj_rgb({r_d, g_d, b_d});
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_r",
        "Adjust red channel by delta",
        std::string("$") + id + " adj_r -10",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_r triggered");
            this->adj_r(String(args[0].c_str()).toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_g",
        "Adjust green channel by delta",
        std::string("$") + id + " adj_g 15",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_g triggered");
            this->adj_g(String(args[0].c_str()).toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_b",
        "Adjust blue channel by delta",
        std::string("$") + id + " adj_b -5",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_b triggered");
            this->adj_b(String(args[0].c_str()).toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_hsv",
        "Set HSV color",
        std::string("$") + id + " set_hsv 75 255 0",
        3,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_hsv triggered");
            uint8_t                h       = String(args[0].c_str()).toInt();
            uint8_t                s       = String(args[1].c_str()).toInt();
            uint8_t                v       = String(args[2].c_str()).toInt();

            std::array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_hue",
        "Set hue channel",
        std::string("$") + id + " set_hue 255",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_hue triggered");
            std::array<uint8_t, 3> current_hsv = get_hsv();
            std::array<uint8_t, 3> new_rgb     = hsv_to_rgb({(uint8_t)String(args[0].c_str()).toInt(), current_hsv[1], current_hsv[2]});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_sat",
        "Set saturation channel",
        std::string("$") + id + " set_sat 0",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_sat triggered");
            std::array<uint8_t, 3> current_hsv = get_hsv();
            std::array<uint8_t, 3> new_rgb     = hsv_to_rgb({current_hsv[0], (uint8_t)String(args[0].c_str()).toInt(), current_hsv[2]});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_val",
        "Set value channel",
        std::string("$") + id + " set_val 255",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_val triggered");
            std::array<uint8_t, 3> current_hsv = get_hsv();
            std::array<uint8_t, 3> new_rgb     = hsv_to_rgb({current_hsv[0], current_hsv[1], (uint8_t)String(args[0].c_str()).toInt()});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_hsv",
        "Adjust HSV color by deltas",
        std::string("$") + id + " adj_hsv 10 -20 5",
        3,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_hsv triggered");
            int h_d = String(args[0].c_str()).toInt();
            int s_d = String(args[1].c_str()).toInt();
            int v_d = String(args[2].c_str()).toInt();

            this->adj_hsv({h_d, s_d, v_d});
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_hue",
        "Adjust hue channel by delta",
        std::string("$") + id + " adj_h -10",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_h triggered");
            this->adj_h(String(args[0].c_str()).toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_sat",
        "Adjust saturation channel by delta",
        std::string("$") + id + " adj_s 15",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_s triggered");
            this->adj_s(String(args[0].c_str()).toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_val",
        "Adjust value channel by delta",
        std::string("$") + id + " adj_v -5",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_v triggered");
            this->adj_v(String(args[0].c_str()).toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_brightness",
        "Set global brightness",
        std::string("$") + id + " set_brightness 255",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_brightness triggered");
            controller.sync_brightness(String(args[0].c_str()).toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_brightness",
        "Adjust global brightness by delta",
        std::string("$") + id + " adj_brightness -10",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_brightness triggered");
            this->adj_brightness(String(args[0].c_str()).toInt());
            controller.sync_brightness(this->get_brightness(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_state",
        "Set on/off state",
        std::string("$") + id + " set_state 0",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_state triggered");
            controller.sync_state(String(args[0].c_str()).toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "toggle_state",
        "If off->on, if on->off",
        std::string("$") + id + " toggle_state",
        0,
        [this, &controller](std::span<const std::string>) {
            DBG_PRINTLN(LedStrip, "CMD: toggle_state triggered");
            controller.sync_state(!get_state(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "turn_on",
        "Turn strip on",
        std::string("$") + id + " turn_on",
        0,
        [this, &controller](std::span<const std::string>) {
            DBG_PRINTLN(LedStrip, "CMD: turn_on triggered");
            controller.sync_state(1, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "turn_off",
        "Turn strip off",
        std::string("$") + id + " turn_off",
        0,
        [this, &controller](std::span<const std::string>) {
            DBG_PRINTLN(LedStrip, "CMD: turn_off triggered");
            controller.sync_state(0, {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_mode",
        "Set LED strip mode",
        std::string("$") + id + " set_mode 0",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_mode triggered");
            controller.sync_mode(String(args[0].c_str()).toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "adj_mode",
        "Adjust mode by delta (next/prev)",
        std::string("$") + id + " adj_mode 1",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_mode triggered");
            this->adj_mode(String(args[0].c_str()).toInt());
            controller.sync_mode(this->get_current_mode_id(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_mode_param",
        "Set a parameter for the current mode",
        std::string("$") + id + " set_mode_param hue 128",
        2,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_mode_param triggered");
            std::string key   = args[0];
            uint16_t    value = String(args[1].c_str()).toInt();

            this->set_mode_param(key, value);
        }
    });

    commands_storage.push_back(Command{
        "adj_mode_param",
        "Adjust a parameter for the current mode by delta",
        std::string("$") + id + " adj_mode_param hue -15",
        2,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: adj_mode_param triggered");
            std::string key   = args[0];
            long        delta = String(args[1].c_str()).toInt();

            this->adj_mode_param(key, delta);
        }
    });

    commands_storage.push_back(Command{
        "get_mode_params",
        "Get parameters of the current mode",
        std::string("$") + id + " get_mode_params",
        0,
        [this, &controller](std::span<const std::string>) {
            DBG_PRINTLN(LedStrip, "CMD: get_mode_params triggered");

            std::stringstream ss;
            ss << "Mode " << static_cast<int>(this->get_current_mode_id())
               << " (" << this->get_current_mode_name() << ") Parameters:\n";

            for (const auto& param : this->mode_controller->get_current_mode_params()) {
                ss << "  - " << param.key << " (" << param.display_name << "): "
                   << param.default_value << " [Range: " << param.min_value << "-" << param.max_value << "]\n";
            }

            controller.serial_port.print(ss.str().c_str());
        }
    });

    commands_storage.push_back(Command{
        "reset_current_mode",
        "Reset current mode parameters to defaults",
        std::string("$") + id + " reset_current_mode",
        0,
        [this, &controller](std::span<const std::string>) {
            DBG_PRINTLN(LedStrip, "CMD: reset_current_mode triggered");
            this->reset_current_mode();

            // Push updated values to the web UI, same pattern as set_mode_param()
            for (const auto& param : this->mode_controller->get_current_mode_params()) {
                // controller.web_interface.sync_param(
                //                     param.key,
                //                     this->get_current_mode_param(param.key)
                //                 );
            }
        }
    });

    commands_storage.push_back(Command{
        "set_length",
        "Set new number of LEDs",
        std::string("$") + id + " set_length 500",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_length triggered");
            controller.sync_length(String(args[0].c_str()).toInt(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back(Command{
        "set_color_order",
        "Set LED color order (RGB/RBG/GRB/GBR/BRG/BGR)",
        std::string("$") + id + " set_color_order GRB",
        1,
        [this, &controller](std::span<const std::string> args) {
            DBG_PRINTLN(LedStrip, "CMD: set_color_order triggered");

            String color_order(args[0].c_str());
            color_order.trim();
            color_order.toUpperCase();

            int8_t new_color_order = -1;

            if (color_order == "RGB") new_color_order = 0;
            else if (color_order == "RBG") new_color_order = 1;
            else if (color_order == "GRB") new_color_order = 2;
            else if (color_order == "GBR") new_color_order = 3;
            else if (color_order == "BRG") new_color_order = 4;
            else if (color_order == "BGR") new_color_order = 5;

            if (new_color_order < 0) {
                controller.serial_port.print("Invalid color order. Use RGB, RBG, GRB, GBR, BRG, or BGR");
                return;
            }

            color_order_index = static_cast<uint8_t>(new_color_order);
            controller.nvs.write<uint8_t>(id, "cfg_colorder", color_order_index);

            controller.serial_port.print(("Color order set to " + std::string(color_order.c_str())).c_str());
        }
    });

    DBG_PRINTLN(LedStrip, "<- LedStrip::LedStrip()");
}

// =============================================================================
// Interface Sync
// =============================================================================
void LedStrip::sync_color(std::array<uint8_t, 3> color) {
    DBG_PRINTF(LedStrip, "-> sync_color(%u, %u, %u)\n", color[0], color[1], color[2]);
    set_rgb(color);
    DBG_PRINTLN(LedStrip, "<- sync_color()");
}

void LedStrip::sync_brightness(uint8_t brightness) {
    DBG_PRINTF(LedStrip, "-> sync_brightness(%u)\n", brightness);
    set_brightness(brightness);
    DBG_PRINTLN(LedStrip, "<- sync_brightness()");
}

void LedStrip::sync_state(bool state) {
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
    this->num_led      = config.num_led;

    DBG_PRINTF(
        LedStrip,
        "Config Num LEDs: %u, Transition Delay: %u\n",
        static_cast<unsigned>(num_led),
        static_cast<unsigned>(config.frame_delay)
    );
    frame_timer = std::make_unique<AsyncTimer<uint8_t>>(config.frame_delay);
    frame_timer->initiate();

    fps_timer = std::make_unique<AsyncTimer<uint8_t>>(config.fps_calc_window_s * 1000);
    fps_timer->initiate();
    fps_calc_window_s = config.fps_calc_window_s;

    brightness        = std::make_unique<Brightness>(config.brightness_transition_delay, 0, 0);
    mode_controller   = std::make_unique<ModeController>(this->leds, this->num_led, config.mode_transition_delay, controller.nvs, "mc");

    DBG_PRINTLN(LedStrip, "<- begin_routines_required()");
}

void LedStrip::begin_routines_init(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_init()");

    controller.serial_port.print("Detailed guide is available here:\nhttps://github.com/maxdokukin/xewe/led-os/TODO");

    if (!controller.serial_port.get_yn("Is LED data line connected to pin GPIO_" + std::to_string(LED_PIN_DATA) + "?")) {
        controller.serial_port.print_header("Pins can not be changed after upload. \nOptions:\\sep1. Upload the version with the correct pin\\sep2. If there is no compiled version with pin that you need, set pins in Config.h and compile yourself");
        while (true);
    }

    if (controller.serial_port.get_yn("Are you using LED with CLK line?")) {
        controller.nvs.write<bool>(id, "cfg_use_clk", true);
        if (!controller.serial_port.get_yn("Is LED CLK line connected to pin GPIO_" + std::to_string(LED_PIN_CLOCK) + "?")) {
            controller.serial_port.print_header("Pins can not be changed after upload.\nOptions:\\sep1. Upload the version with the correct pin\\sep2. If there is no compiled version with pin that you need, set pins in Config.h and compile yourself");
            while (true);
        }
    } else {
        controller.nvs.write<bool>(id, "cfg_use_clk", false);
    }

    std::vector<std::string> chipset_names;
    for (const auto& entry : LedStrip::LED_CHIPSET_TABLE) {
        chipset_names.push_back(entry.name);
    }

    uint8_t selected_chip_id = controller.serial_port.get_menu_choice("What is your LED Chip?", chipset_names) - 1;
    controller.nvs.write<uint8_t>(id, "cfg_chip", selected_chip_id);
    if (!set_leds_chipset(LedStrip::LED_CHIPSET_TABLE[selected_chip_id].value)) {
        controller.serial_port.print("Failed to initialize selected led chip");
        controller.system.restart();
    }
    FastLED.setBrightness(255);

    switch (controller.serial_port.get_menu_choice("What is LED Voltage?", {"5V", "12V", "24V"})) {
        case 1: controller.nvs.write<uint8_t>(id, "cfg_voltage", 5); break;
        case 2: controller.nvs.write<uint8_t>(id, "cfg_voltage", 12); break;
        case 3: controller.nvs.write<uint8_t>(id, "cfg_voltage", 24); break;
    }

    if (controller.serial_port.get_yn("Do you have parallel LED strips attached to data line?")) {
        uint16_t parallel_led_strips_count = controller.serial_port.get_int(
            "How many LED strips do you have in parallel?",
            1,
            LED_STRIP_NUM_LEDS_MAX
        );

        controller.nvs.write<uint16_t>(id, "cfg_lines", parallel_led_strips_count);

        for (uint16_t i = 0; i < parallel_led_strips_count; ++i) {
            uint16_t leds_per_line = controller.serial_port.get_int(
                "How many LEDs are connected on parallel line #" + std::to_string(i + 1) + "?",
                1,
                LED_STRIP_NUM_LEDS_MAX
            );

            num_led = max(num_led, leds_per_line);

            controller.nvs.write<uint16_t>(
                id,
                "cfg_l_" + std::to_string(i) + "_cnt",
                leds_per_line
            );
        }
    } else {
        controller.nvs.write<uint16_t>(id, "cfg_lines", 0);
        num_led = controller.serial_port.get_int("How many LEDs do you have connected?", 0, LED_STRIP_NUM_LEDS_MAX);
    }

    set_rgb({0, 255, 0});
    set_brightness(50);
    set_mode(0);
    turn_on();

    controller.serial_port.print_header("Color Order Calibration");
    run_with_dots([this] { loop(); }, (float)mode_controller->get_mode_transition_delay() * 1.2f);

    char    color_order[3] = {'b', 'b', 'b'};

    uint8_t color_visible  = controller.serial_port.get_menu_choice(
        "What color are LEDs now?",
        {"Red", "Green", "Blue", "Other"}
    );

    if (color_visible == 4) {
        controller.serial_port.print_header("Double check pins, and LED chip type.\nNote that RGBW is not supported.");
        controller.system.restart();
    }
    color_order[color_visible - 1] = 'g';

    controller.serial_port.print("Changing color", "");
    set_rgb({255, 0, 0});
    run_with_dots([this] { loop(); }, (float)mode_controller->get_mode_transition_delay() * 1.2f);

    color_visible = controller.serial_port.get_menu_choice(
        "What color are LEDs now?",
        {"Red", "Green", "Blue"}
    );
    color_order[color_visible - 1] = 'r';

    controller.serial_port.print("Setting color order", "");
    turn_off();
    run_with_dots([this] { loop(); }, (float)mode_controller->get_mode_transition_delay() * 1.2f);

    if (color_order[0] == 'r' && color_order[1] == 'g' && color_order[2] == 'b') color_order_index = 0;      // RGB
    else if (color_order[0] == 'r' && color_order[1] == 'b' && color_order[2] == 'g') color_order_index = 1; // RBG
    else if (color_order[0] == 'g' && color_order[1] == 'r' && color_order[2] == 'b') color_order_index = 2; // GRB
    else if (color_order[0] == 'g' && color_order[1] == 'b' && color_order[2] == 'r') color_order_index = 3; // GBR
    else if (color_order[0] == 'b' && color_order[1] == 'r' && color_order[2] == 'g') color_order_index = 4; // BRG
    else if (color_order[0] == 'b' && color_order[1] == 'g' && color_order[2] == 'r') color_order_index = 5; // BGR
    controller.nvs.write<uint8_t>(id, "cfg_colorder", color_order_index);

    turn_on();
    set_rgb({0, 255, 0});

    controller.serial_port.print("LED setup success!");

    DBG_PRINTLN(LedStrip, "<- begin_routines_init()");
}

//// HARDCODED DEV VERISON
// void LedStrip::begin_routines_init(const ModuleConfig& cfg) {
//     DBG_PRINTLN(LedStrip, "-> begin_routines_init()");
//
//      controller.nvs.write<bool>(id, "cfg_use_clk", false);
//
//     uint8_t selected_chip_id = 42;
//      controller.nvs.write<uint8_t>(id, "cfg_chip", selected_chip_id);
//     set_leds_chipset(LedStrip::LED_CHIPSET_TABLE[selected_chip_id].value);
//
//     FastLED.setBrightness(255);
//      controller.nvs.write<uint8_t>(id, "cfg_voltage", 5);
//      controller.nvs.write<uint16_t>(id, "cfg_lines", 0);
//
//     num_led = 1;
//
//     color_order_index = 2;
//      controller.nvs.write<uint8_t>(id, "cfg_colorder", color_order_index);
//
//     // controller.sync_all(
//         {0, 255, 0},
//         50,
//         1,
//         0,
//         num_led,
//         {true, true, false, false, false} //only write to led and nvs
//     );
//
//     DBG_PRINTLN(LedStrip, "<- begin_routines_init()");
// }

void LedStrip::begin_routines_regular(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_regular()");

    // load params from memory
    uint8_t selected_chip_id = controller.nvs.read<uint8_t>(id, "cfg_chip");
    set_leds_chipset(LedStrip::LED_CHIPSET_TABLE[selected_chip_id].value);
    FastLED.setBrightness(255);
    color_order_index                   = controller.nvs.read<uint8_t>(id, "cfg_colorder", 0);

    //      controller.nvs.sync_from_memory({true, false, false, false, false});
    const std::vector<uint8_t> rgb_blob = controller.nvs.read_blob(id, "rgb");
    set_rgb({rgb_blob[0], rgb_blob[1], rgb_blob[2]});
    set_brightness(controller.nvs.read<uint8_t>(id, "brightness"));
    set_mode(controller.nvs.read<uint8_t>(id, "mode_id"));
    set_state(controller.nvs.read<bool>(id, "state"));

    DBG_PRINTLN(LedStrip, "<- begin_routines_regular()");
}

void LedStrip::begin_routines_common(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_common()");
    controller.serial_port.print("Shining the light", "");
    run_with_dots([this] { loop(); }, (float)mode_controller->get_mode_transition_delay() * 1.2f);
    DBG_PRINTLN(LedStrip, "<- begin_routines_common()");
}

void LedStrip::loop() {
    if (frame_timer->is_not_done()) return;
    frame_timer->reset();
    frame_timer->initiate();

    mode_controller->loop();
    set_all(leds);

    fps_counter++;
    if (fps_timer->is_done()) {
        fps_calculated = fps_counter / fps_calc_window_s;
        fps_counter    = 0;
        fps_timer->reset();
        fps_timer->initiate();
    }
}

void LedStrip::reset(const bool verbose,
                     const bool do_restart,
                     const bool keep_enabled) {
    DBG_PRINTLN(LedStrip, "-> reset()");
    controller.sync_all(
        {0, 255, 0},
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

std::string LedStrip::status(const bool verbose) const {
    DBG_PRINTLN(LedStrip, "-> status()");

    // 1. Fetch configurations upfront
    const bool     use_clk          = controller.nvs.read<bool>(id, "cfg_use_clk", false);
    const uint8_t  configured_chip  = controller.nvs.read<uint8_t>(id, "cfg_chip", 0);
    const uint8_t  configured_v     = controller.nvs.read<uint8_t>(id, "cfg_voltage", 5);
    const uint8_t  configured_co    = controller.nvs.read<uint8_t>(id, "cfg_colorder", color_order_index);
    const uint16_t configured_lines = controller.nvs.read<uint16_t>(id, "cfg_lines", 0);

    const float    current_v        = (configured_v > 0) ? static_cast<float>(configured_v) : 5.0f;
    const bool     is_on            = get_state();
    const uint16_t signal_length    = get_length();

    // 2. Streamlined lambdas
    auto           get_chip_name    = [](uint8_t chip_id) -> std::string {
        const size_t count = sizeof(LedStrip::LED_CHIPSET_TABLE) / sizeof(LedStrip::LED_CHIPSET_TABLE[0]);
        return (chip_id < count) ? LedStrip::LED_CHIPSET_TABLE[chip_id].name : "Unknown";
    };

    auto get_color_order_name = [](uint8_t idx) -> const char* {
        static constexpr const char* NAMES[] = {"RGB", "RBG", "GRB", "GBR", "BRG", "BGR"};
        const size_t                 count   = sizeof(NAMES) / sizeof(NAMES[0]);
        return (idx < count) ? NAMES[idx] : "Unknown";
    };

    // 3. Unified line processing (handles both 0 lines and N lines dynamically)
    uint32_t          total_physical_leds = 0;
    uint32_t          total_power_mw      = 0;
    const uint16_t    loop_count          = (configured_lines == 0) ? 1 : configured_lines;

    std::stringstream hw_lines_stream;
    std::stringstream pwr_lines_stream;

    for (uint16_t i = 0; i < loop_count; ++i) {
        uint16_t line_len = num_led; // Default for 0 configured lines

        if (configured_lines > 0) {
            line_len = controller.nvs.read<uint16_t>(id, "cfg_l_" + std::to_string(i) + "_cnt", 0);
            hw_lines_stream << "    Line " << (i + 1) << " Length:    " << line_len << "\n";
        } else {
            hw_lines_stream << "    Length:           " << line_len << "\n";
        }

        total_physical_leds += line_len;

        uint32_t line_power_mw = 0;
        if (is_on && line_len > 0) {
            const uint16_t powered_len = (line_len < signal_length) ? line_len : signal_length;
            line_power_mw              = calculate_unscaled_power_mW(leds, powered_len);
        }
        total_power_mw += line_power_mw;

        const float power_w   = line_power_mw / 1000.0f;
        const float current_a = power_w / current_v; // current_v is safely > 0

        pwr_lines_stream << "    Line " << (configured_lines > 0 ? std::to_string(i + 1) : "1")
                         << ":           " << power_w << " W, " << current_a << " A\n";
    }

    // 4. Build the final output efficiently
    std::stringstream ss;
    ss << "Hardware:\n"
       << "    Chip:             " << get_chip_name(configured_chip) << "\n"
       << "    Data Pin:         GPIO_" << static_cast<int>(LED_PIN_DATA) << "\n";

    if (use_clk) {
        ss << "    Clock Pin:        GPIO_" << static_cast<int>(LED_PIN_CLOCK) << "\n";
    }

    ss << "    Voltage:          " << static_cast<int>(configured_v) << " V\n"
       << "    Color Order:      " << get_color_order_name(configured_co) << "\n";

    if (configured_lines == 0) {
        ss << hw_lines_stream.str()
           << "    Max Length:       " << LED_STRIP_NUM_LEDS_MAX << "\n\n";
    } else {
        ss << "    Parallel Lines:   " << configured_lines << "\n"
           << hw_lines_stream.str()
           << "    Total LEDs:       " << total_physical_leds << "\n"
           << "    Max LEDs/line:    " << LED_STRIP_NUM_LEDS_MAX << "\n\n";
    }

    ss << "Live State:\n"
       << "    FPS:              " << fps_calculated << "\n"
       << "    Brightness:       " << static_cast<int>(get_brightness()) << "/255\n"
       << "    Power State:      " << (is_on ? "ON" : "OFF") << "\n"
       << "    Color (RGB):      (" << static_cast<int>(get_r()) << ", "
       << static_cast<int>(get_g()) << ", "
       << static_cast<int>(get_b()) << ")\n\n";

    const float total_power_w = total_power_mw / 1000.0f;
    ss << "Power:\n"
       << "    Voltage:          " << current_v << " V\n"
       << pwr_lines_stream.str()
       << "    Total:            " << total_power_w << " W, "
       << (total_power_w / current_v) << " A\n\n"
       << "Mode: [" << static_cast<int>(get_current_mode_id()) << "] "
       << get_current_mode_name() << "\n";

    // 5. Mode Parameters
    auto params = mode_controller->get_current_mode_params();
    if (params.empty()) {
        ss << "    (No parameters for this mode)\n";
    } else {
        for (const auto& param : params) {
            ss << "    - " << param.key << ": " << mode_controller->get_current_mode_param(param.key)
               << " [" << param.min_value << "-" << param.max_value << "] (" << param.display_name << ")\n";
        }
    }

    std::string status_string = ss.str();

    if (verbose)
        controller.serial_port.print(status_string.c_str());

    DBG_PRINTLN(LedStrip, "<- status()");
    return status_string;
}

// =============================================================================
// Custom Methods: Color
// =============================================================================
void LedStrip::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    mode_controller->set_rgb(new_rgb);
    controller.nvs.write_blob(id, "rgb", new_rgb);
}

void LedStrip::set_rgb(const uint8_t r,
                       const uint8_t g,
                       const uint8_t b) {
    set_rgb({r, g, b});
}

void LedStrip::set_r(const uint8_t r) {
    set_rgb({r, mode_controller->get_rgb()[1], mode_controller->get_rgb()[2]});
}

void LedStrip::set_g(const uint8_t g) {
    set_rgb({mode_controller->get_rgb()[0], g, mode_controller->get_rgb()[2]});
}

void LedStrip::set_b(const uint8_t b) {
    set_rgb({mode_controller->get_rgb()[0], mode_controller->get_rgb()[1], b});
}

void LedStrip::set_hsv(const std::array<uint8_t, 3> new_hsv) {
    mode_controller->set_hsv(new_hsv);
    controller.nvs.write_blob(id, "rgb", get_rgb());
}

void LedStrip::set_h(const uint8_t h) {
    std::array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({h, old_hsv[1], old_hsv[2]});
}

void LedStrip::set_s(const uint8_t s) {
    std::array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({old_hsv[0], s, old_hsv[2]});
}

void LedStrip::set_v(const uint8_t v) {
    std::array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({old_hsv[0], old_hsv[1], v});
}

void LedStrip::adj_rgb(const std::array<int, 3> rgb_delta) {
    std::array<uint8_t, 3> adjusted_rgb = get_rgb();

    for (int i = 0; i < 3; i++)
        adjusted_rgb[i] = static_cast<uint8_t>(std::clamp<int>(adjusted_rgb[i] + rgb_delta[i], 0, 255));

    set_rgb(adjusted_rgb);
}

void LedStrip::adj_r(const int r_delta) {
    adj_rgb({r_delta, 0, 0});
}

void LedStrip::adj_g(const int g_delta) {
    adj_rgb({0, g_delta, 0});
}

void LedStrip::adj_b(const int b_delta) {
    adj_rgb({0, 0, b_delta});
}

void LedStrip::adj_hsv(const std::array<int, 3> hsv_delta) {
    std::array<uint8_t, 3> adjusted_hsv = get_hsv();

    for (int i = 0; i < 3; i++) {
        int adj_value = adjusted_hsv[i] + hsv_delta[i];

        if (i == 0) { // Hue wraps around 0-255
            while (adj_value < 0) adj_value += 256;
            adjusted_hsv[i] = static_cast<uint8_t>(adj_value % 256);
        } else { // Saturation and Value constrain 0-255
            adjusted_hsv[i] = static_cast<uint8_t>(std::clamp<int>(adj_value, 0, 255));
        }
    }

    set_hsv(adjusted_hsv);
}

void LedStrip::adj_hsv(const uint8_t h_delta,
                       const uint8_t s_delta,
                       const uint8_t v_delta) {
    adj_hsv({h_delta, s_delta, v_delta});
}

void LedStrip::adj_h(const int h_delta) {
    adj_hsv({h_delta, 0, 0});
}

void LedStrip::adj_s(const int s_delta) {
    adj_hsv({0, s_delta, 0});
}

void LedStrip::adj_v(const int v_delta) {
    adj_hsv({0, 0, v_delta});
}

std::array<uint8_t, 3> LedStrip::get_rgb() const {
    return mode_controller->get_rgb();
}

uint8_t LedStrip::get_r() const {
    return get_rgb()[0];
}

uint8_t LedStrip::get_g() const {
    return get_rgb()[1];
}

uint8_t LedStrip::get_b() const {
    return get_rgb()[2];
}

std::array<uint8_t, 3> LedStrip::get_hsv() const {
    return rgb_to_hsv(get_rgb());
}

uint8_t LedStrip::get_h() const {
    return get_hsv()[0];
}

uint8_t LedStrip::get_s() const {
    return get_hsv()[1];
}

uint8_t LedStrip::get_v() const {
    return get_hsv()[2];
}

// =============================================================================
// Custom Methods: Brightness
// =============================================================================
void LedStrip::set_brightness(const uint8_t new_brightness) {
    brightness->set_brightness(new_brightness);
    controller.nvs.write<uint8_t>(id, "brightness", get_brightness());
}

void LedStrip::adj_brightness(const int brightness_delta) {
    int new_brightness = brightness->get_target_value() + brightness_delta;
    set_brightness(static_cast<uint8_t>(std::clamp<int>(new_brightness, 0, 255)));
    controller.nvs.write<uint8_t>(id, "brightness", get_brightness());
}

uint8_t LedStrip::get_brightness() const {
    return brightness->get_last_brightness();
}

// =============================================================================
// Custom Methods: State
// =============================================================================
void LedStrip::set_state(const bool state) {
    if (state) {
        turn_on();
    } else {
        turn_off();
    }
    controller.nvs.write<bool>(id, "state", get_state());
}

void LedStrip::toggle_state() {
    if (get_state()) {
        turn_off();
    } else {
        turn_on();
    }
    controller.nvs.write<bool>(id, "state", get_state());
}

void LedStrip::turn_on() {
    brightness->turn_on();
    controller.nvs.write<bool>(id, "state", get_state());
}

void LedStrip::turn_off() {
    brightness->turn_off();
    controller.nvs.write<bool>(id, "state", get_state());
}

bool LedStrip::get_state() const {
    return brightness->get_state();
}

// =============================================================================
// Custom Methods: Mode
// =============================================================================
void LedStrip::set_mode(const uint8_t new_mode) {
    mode_controller->set_mode(new_mode);
    controller.sync_color(get_rgb(), {true, true, true, true, true});
    controller.nvs.write<uint8_t>(id, "mode_id", new_mode);
}

void LedStrip::adj_mode(const int mode_delta) {
    uint8_t new_mode = static_cast<uint8_t>(std::clamp<int>(get_current_mode_id() + mode_delta, 0, 255));
    set_mode(new_mode);
}

void LedStrip::set_mode_param(std::string_view key,
                              const uint16_t value) {
    if (mode_controller->set_mode_param(key, value)) {
        controller.sync_color(get_rgb(), {true, true, true, true, true});
        // controller.web_interface.sync_param(key, value);
    }
}

void LedStrip::adj_mode_param(std::string_view key,
                              const long value_delta) {
    if (mode_controller->adj_mode_param(key, value_delta)) {
        controller.sync_color(get_rgb(), {true, true, true, true, true});
        // controller.web_interface.sync_param(key, mode_controller->get_current_mode_param(key));
    }
}

uint8_t LedStrip::get_current_mode_id() const {
    return mode_controller->get_current_mode_id();
}

std::string_view LedStrip::get_current_mode_name() const {
    return mode_controller->get_current_mode_name();
}

uint16_t LedStrip::get_current_mode_param(std::string_view key) const {
    return mode_controller->get_current_mode_param(key);
}

void LedStrip::reset_current_mode() {
    mode_controller->reset_current_mode();
}

std::string LedStrip::get_all_modes_json() const {
    return mode_controller->get_all_modes_json();
}

// =============================================================================
// Custom Methods: Length
// =============================================================================
void LedStrip::set_length(const uint16_t length) {
    DBG_PRINTF(LedStrip, "-> set_length(%u)\n", length);
    if (length > LED_STRIP_NUM_LEDS_MAX) {
        controller.serial_port.print("That's too many. Max supported: " + std::to_string(LED_STRIP_NUM_LEDS_MAX) + " LEDs");
        DBG_PRINTLN(LedStrip, "Error: set_length exceeded MAX limit");
        return;
    }

    set_black();
    num_led = length;
    mode_controller->set_length(length);
    controller.nvs.write<uint16_t>(id, "length", length);
    DBG_PRINTLN(LedStrip, "<- set_length()");
}

uint16_t LedStrip::get_length() const {
    return num_led;
}

// =============================================================================
// Custom Methods: Fill
// =============================================================================

void LedStrip::set_pixel(uint16_t i,
                         std::array<uint8_t, 3> color_rgb) {
    if (i < num_led) {
        std::array<uint8_t, 3> dimmed_color = brightness->get_dimmed_color(color_rgb);

        switch (color_order_index) {
            case 0: // RGB
                leds[i] = CRGB(dimmed_color[0], dimmed_color[1], dimmed_color[2]);
                break;

            case 1: // RBG
                leds[i] = CRGB(dimmed_color[0], dimmed_color[2], dimmed_color[1]);
                break;

            case 2: // GRB
                leds[i] = CRGB(dimmed_color[1], dimmed_color[0], dimmed_color[2]);
                break;

            case 3: // GBR
                leds[i] = CRGB(dimmed_color[1], dimmed_color[2], dimmed_color[0]);
                break;

            case 4: // BRG
                leds[i] = CRGB(dimmed_color[2], dimmed_color[0], dimmed_color[1]);
                break;

            case 5: // BGR
                leds[i] = CRGB(dimmed_color[2], dimmed_color[1], dimmed_color[0]);
                break;
        }
    }
}

void LedStrip::set_all(CRGB* new_leds) {
    if (new_leds != nullptr) {
        for (uint16_t i = 0; i < num_led; i++) {
            set_pixel(i, {new_leds[i].r, new_leds[i].g, new_leds[i].b});
        }
        FastLED.show();
    }
}

void LedStrip::set_all(const uint8_t r,
                       const uint8_t g,
                       const uint8_t b) {
    for (uint16_t i = 0; i < num_led; i++) {
        set_pixel(i, {r, g, b});
    }
    FastLED.show();
}

void LedStrip::set_black() {
    DBG_PRINTLN(LedStrip, "-> set_black()");
    fill_solid(leds, num_led, CRGB::Black);
    FastLED.show();
    DBG_PRINTLN(LedStrip, "<- set_black()");
}

bool LedStrip::set_leds_chipset(const LedStrip::LEDChipset chipset) {
    switch (chipset) {
        case LEDChipset::APA102: FastLED.addLeds<APA102, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA102HD: FastLED.addLeds<APA102HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA104: FastLED.addLeds<APA104, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA106: FastLED.addLeds<APA106, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::DOTSTAR: FastLED.addLeds<DOTSTAR, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::DOTSTARHD: FastLED.addLeds<DOTSTARHD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GE8822: FastLED.addLeds<GE8822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GS1903: FastLED.addLeds<GS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GW6205: FastLED.addLeds<GW6205, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GW6205_400KHZ: FastLED.addLeds<GW6205_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::HD107: FastLED.addLeds<HD107, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::HD107HD: FastLED.addLeds<HD107HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD1886: FastLED.addLeds<LPD1886, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD1886_8BIT: FastLED.addLeds<LPD1886_8BIT, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD6803: FastLED.addLeds<LPD6803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD8806: FastLED.addLeds<LPD8806, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::NEOPIXEL: FastLED.addLeds<NEOPIXEL, LED_PIN_DATA>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::P9813: FastLED.addLeds<P9813, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::PL9823: FastLED.addLeds<PL9823, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK6812: FastLED.addLeds<SK6812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK6822: FastLED.addLeds<SK6822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK9822: FastLED.addLeds<SK9822, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK9822HD: FastLED.addLeds<SK9822HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16703: FastLED.addLeds<SM16703, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16716: FastLED.addLeds<SM16716, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16824E: FastLED.addLeds<SM16824E, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1803: FastLED.addLeds<TM1803, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1804: FastLED.addLeds<TM1804, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1809: FastLED.addLeds<TM1809, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1812: FastLED.addLeds<TM1812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1829: FastLED.addLeds<TM1829, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1903: FastLED.addLeds<UCS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1903B: FastLED.addLeds<UCS1903B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1904: FastLED.addLeds<UCS1904, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1912: FastLED.addLeds<UCS1912, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS2903: FastLED.addLeds<UCS2903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2801: FastLED.addLeds<WS2801, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2803: FastLED.addLeds<WS2803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2811: FastLED.addLeds<WS2811, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2811_400KHZ: FastLED.addLeds<WS2811_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2812: FastLED.addLeds<WS2812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2812B: FastLED.addLeds<WS2812B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2813: FastLED.addLeds<WS2813, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2815: FastLED.addLeds<WS2815, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2816: FastLED.addLeds<WS2816, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2852: FastLED.addLeds<WS2852, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;

        default: return false;
    }
}
