/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
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
        "adj_rgb",
        "Adjust RGB color by deltas",
        string("$") + lower(module_name) + " adj_rgb 10 -20 5",
        3,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_rgb triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;
            int i2 = args.indexOf(' ', i1 + 1);
            if (i2 == -1) return;

            int r_d = args.substring(0, i1).toInt();
            int g_d = args.substring(i1 + 1, i2).toInt();
            int b_d = args.substring(i2 + 1).toInt();

            this->adj_rgb({r_d, g_d, b_d});
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_r",
        "Adjust red channel by delta",
        string("$") + lower(module_name) + " adj_r -10",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_r triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_r(args.toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_g",
        "Adjust green channel by delta",
        string("$") + lower(module_name) + " adj_g 15",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_g triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_g(args.toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_b",
        "Adjust blue channel by delta",
        string("$") + lower(module_name) + " adj_b -5",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_b triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_b(args.toInt());
            controller.sync_color(this->get_rgb(), {true, true, true, true, true});
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

            array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
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
            array<uint8_t, 3> new_rgb = hsv_to_rgb({(uint8_t)args.toInt(), current_hsv[1], current_hsv[2]});
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
            array<uint8_t, 3> new_rgb = hsv_to_rgb({current_hsv[0], (uint8_t)args.toInt(), current_hsv[2]});
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
            array<uint8_t, 3> new_rgb = hsv_to_rgb({current_hsv[0], current_hsv[1], (uint8_t)args.toInt()});
            controller.sync_color(new_rgb, {true, true, true, true, true});
        }
    });


    commands_storage.push_back({
        "adj_hsv",
        "Adjust HSV color by deltas",
        string("$") + lower(module_name) + " adj_hsv 10 -20 5",
        3,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_hsv triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;
            int i2 = args.indexOf(' ', i1 + 1);
            if (i2 == -1) return;

            int h_d = args.substring(0, i1).toInt();
            int s_d = args.substring(i1 + 1, i2).toInt();
            int v_d = args.substring(i2 + 1).toInt();

            this->adj_hsv({h_d, s_d, v_d});
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_hue",
        "Adjust hue channel by delta",
        string("$") + lower(module_name) + " adj_h -10",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_h triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_h(args.toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_sat",
        "Adjust saturation channel by delta",
        string("$") + lower(module_name) + " adj_s 15",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_s triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_s(args.toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "adj_val",
        "Adjust value channel by delta",
        string("$") + lower(module_name) + " adj_v -5",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_v triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_v(args.toInt());
            controller.sync_color(hsv_to_rgb(this->get_hsv()), {true, true, true, true, true});
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
        "adj_brightness",
        "Adjust global brightness by delta",
        string("$") + lower(module_name) + " adj_brightness -10",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_brightness triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_brightness(args.toInt());
            controller.sync_brightness(this->get_brightness(), {true, true, true, true, true});
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
        "adj_mode",
        "Adjust mode by delta (next/prev)",
        string("$") + lower(module_name) + " adj_mode 1",
        1,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_mode triggered");
            String args(args_sv.data(), args_sv.length());
            this->adj_mode(args.toInt());
            controller.sync_mode(this->get_current_mode_id(), {true, true, true, true, true});
        }
    });

    commands_storage.push_back({
        "set_mode_param",
        "Set a parameter for the current mode",
        string("$") + lower(module_name) + " set_mode_param hue 128",
        2,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: set_mode_param triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;

            std::string key = args.substring(0, i1).c_str();
            uint16_t value = args.substring(i1 + 1).toInt();

            this->set_mode_param(key, value);
        }
    });

    commands_storage.push_back({
        "adj_mode_param",
        "Adjust a parameter for the current mode by delta",
        string("$") + lower(module_name) + " adj_mode_param hue -15",
        2,
        [this, &controller](string_view args_sv) {
            DBG_PRINTLN(LedStrip, "CMD: adj_mode_param triggered");
            String args(args_sv.data(), args_sv.length());
            int i1 = args.indexOf(' ');
            if (i1 == -1) return;

            std::string key = args.substring(0, i1).c_str();
            long delta = args.substring(i1 + 1).toInt();

            this->adj_mode_param(key, delta);
        }
    });

    commands_storage.push_back({
        "get_mode_params",
        "Get parameters of the current mode",
        string("$") + lower(module_name) + " get_mode_params",
        0,
        [this, &controller](string_view args_sv) {
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

    DBG_PRINTF(LedStrip, "Config Num LEDs: %u, Transition Delay: %u\n", num_led);

    frame_timer = make_unique<AsyncTimer<uint8_t>>(config.frame_delay);
    brightness = make_unique<Brightness>(config.brightness_transition_delay, 0, 0);
    mode_controller = std::make_unique<ModeController>(this->leds, this->num_led, config.mode_transition_delay);

    frame_timer->initiate();
    DBG_PRINTLN(LedStrip, "<- begin_routines_required()");
}

void LedStrip::begin_routines_init(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_init()");

    controller.serial_port.print("Detailed guide is available here:\nhttps://github.com/maxdokukin/xewe/led-os/TODO");

    if (!controller.serial_port.get_yn("Is LED data line connected to pin GPIO_" + std::to_string(LED_PIN_DATA) + "?")) {
        controller.serial_port.print_header("Pins can not be changed after upload. \nOptions:\\sep1. Upload the version with the correct pin\\sep2. If there is no compiled version with pin that you need, set pins in Config.h and compile yourself");
        while(true);
    }

    if (controller.serial_port.get_yn("Are you using LED with CLK line?")) {
        controller.nvs.write_bool(nvs_key, "cfg_use_clk", true);
        if (!controller.serial_port.get_yn("Is LED CLK line connected to pin GPIO_" + std::to_string(LED_PIN_CLOCK) + "?")) {
            controller.serial_port.print_header("Pins can not be changed after upload.\nOptions:\\sep1. Upload the version with the correct pin\\sep2. If there is no compiled version with pin that you need, set pins in Config.h and compile yourself");
            while(true);
        }
    }
    else {
        controller.nvs.write_bool(nvs_key, "cfg_use_clk", false);
    }

    std::vector<std::string> chipset_names;
    for (const auto& entry : LedStrip::LED_CHIPSET_TABLE) {
        chipset_names.push_back(entry.name);
    }

    uint8_t selected_chip_id = controller.serial_port.get_menu_choice("What is your LED Chip?", chipset_names) - 1;
    controller.nvs.write_uint8(nvs_key, "cfg_chip", selected_chip_id);
    if(!set_leds_chipset(LedStrip::LED_CHIPSET_TABLE[selected_chip_id].value)) {
        controller.serial_port.print("Failed to initialize selected led chip");
        controller.system.restart();
    }
    FastLED.setBrightness(255);

    switch (controller.serial_port.get_menu_choice("What is LED Voltage?", {"5V", "12V", "24V"})) {
        case 1: controller.nvs.write_uint8(nvs_key, "cfg_voltage", 5);  break;
        case 2: controller.nvs.write_uint8(nvs_key, "cfg_voltage", 12); break;
        case 3: controller.nvs.write_uint8(nvs_key, "cfg_voltage", 24); break;
    }

    if (controller.serial_port.get_yn("Do you have parallel LED strips attached to data line?")) {
        uint16_t parallel_led_strips_count = controller.serial_port.get_int(
            "How many LED strips do you have in parallel?",
            1,
            LED_STRIP_NUM_LEDS_MAX
        );

        controller.nvs.write_uint16(nvs_key, "cfg_lines", parallel_led_strips_count);

        for (uint16_t i = 0; i < parallel_led_strips_count; ++i) {
            uint16_t leds_per_line = controller.serial_port.get_int(
                "How many LEDs are connected on parallel line #" + std::to_string(i + 1) + "?",
                1,
                LED_STRIP_NUM_LEDS_MAX
            );

            num_led = max(num_led, leds_per_line);

            controller.nvs.write_uint16(
                nvs_key,
                "cfg_l_" + std::to_string(i) + "_cnt",
                leds_per_line
            );
        }
    } else {
        controller.nvs.write_uint16(nvs_key, "cfg_lines", 0);
        num_led = controller.serial_port.get_int("How many LEDs do you have connected?", 0, LED_STRIP_NUM_LEDS_MAX);
    }

    controller.sync_all(
        {0, 255, 0},
        50,
        1,
        0,
        num_led,
        {true, true, false, false, false} //only write to led and nvs
    );

    controller.serial_port.print_header("Color Order Calibration");
    run_with_dots([this] { loop(); }, (float) mode_controller->get_mode_transition_delay() * 1.2f);

    char color_order[3] = {'b', 'b', 'b'};

    uint8_t color_visible = controller.serial_port.get_menu_choice(
        "What color are LEDs now?",
        {"Red", "Green", "Blue", "Other"}
    );

    if (color_visible == 4) {
        controller.serial_port.print_header("Double check pins, and LED chip type.\nNote that RGBW is not supported.");
        controller.system.restart();
    }
    color_order[color_visible - 1] = 'g';

    controller.serial_port.print("Changing color");
    set_rgb({255, 0, 0});
    run_with_dots([this] { loop(); }, (float) mode_controller->get_mode_transition_delay() * 1.2f);

    color_visible = controller.serial_port.get_menu_choice(
        "What color are LEDs now?",
        {"Red", "Green", "Blue"}
    );
    color_order[color_visible - 1] = 'r';

    controller.serial_port.print("Setting color order");
    turn_off();
    run_with_dots([this] { loop(); }, (float) mode_controller->get_mode_transition_delay() * 1.2f);

    if      (color_order[0]=='r' && color_order[1]=='g' && color_order[2]=='b') color_order_index = 0; // RGB
    else if (color_order[0]=='r' && color_order[1]=='b' && color_order[2]=='g') color_order_index = 1; // RBG
    else if (color_order[0]=='g' && color_order[1]=='r' && color_order[2]=='b') color_order_index = 2; // GRB
    else if (color_order[0]=='g' && color_order[1]=='b' && color_order[2]=='r') color_order_index = 3; // GBR
    else if (color_order[0]=='b' && color_order[1]=='r' && color_order[2]=='g') color_order_index = 4; // BRG
    else if (color_order[0]=='b' && color_order[1]=='g' && color_order[2]=='r') color_order_index = 5; // BGR
    controller.nvs.write_uint8(nvs_key, "cfg_colorder", color_order_index);

    turn_on();
    set_rgb({0, 255, 0});
    controller.serial_port.print("LED setup success!");

    DBG_PRINTLN(LedStrip, "<- begin_routines_init()");
}

void LedStrip::begin_routines_regular(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_regular()");
    // load params from memory
    uint8_t selected_chip_id = controller.nvs.read_uint8(nvs_key, "cfg_chip");
    set_leds_chipset(LedStrip::LED_CHIPSET_TABLE[selected_chip_id].value);
    FastLED.setBrightness(255);
    color_order_index = controller.nvs.read_uint8(nvs_key, "cfg_colorder", 0);

    controller.nvs.sync_from_memory({true, false, false, false, false});
    DBG_PRINTLN(LedStrip, "<- begin_routines_regular()");
}

void LedStrip::begin_routines_common(const ModuleConfig& cfg) {
    DBG_PRINTLN(LedStrip, "-> begin_routines_common()");
    controller.serial_port.print("Setting up LED lights");
    run_with_dots([this] { loop(); }, (float) mode_controller->get_mode_transition_delay() * 1.2f);
    DBG_PRINTLN(LedStrip, "<- begin_routines_common()");
}

void LedStrip::loop() {
    // Note: No logging in loop() to prevent Serial spam and frame drops
    if (frame_timer->is_not_done()) return;
    frame_timer->reset();
    frame_timer->initiate();

    mode_controller->loop();
    set_all(leds);

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
std::string LedStrip::status(const bool verbose) const {
    DBG_PRINTLN(LedStrip, "-> status()");

    // 1. Fetch configurations upfront
    const bool     use_clk          = controller.nvs.read_bool(nvs_key, "cfg_use_clk", false);
    const uint8_t  configured_chip  = controller.nvs.read_uint8(nvs_key, "cfg_chip", 0);
    const uint8_t  configured_v     = controller.nvs.read_uint8(nvs_key, "cfg_voltage", 5);
    const uint8_t  configured_co    = controller.nvs.read_uint8(nvs_key, "cfg_colorder", color_order_index);
    const uint16_t configured_lines = controller.nvs.read_uint16(nvs_key, "cfg_lines", 0);

    const float current_v = (configured_v > 0) ? static_cast<float>(configured_v) : 5.0f;
    const bool  is_on     = get_state();
    const uint16_t signal_length = get_length();

    // 2. Streamlined lambdas
    auto get_chip_name = [](uint8_t chip_id) -> std::string {
        const size_t count = sizeof(LedStrip::LED_CHIPSET_TABLE) / sizeof(LedStrip::LED_CHIPSET_TABLE[0]);
        return (chip_id < count) ? LedStrip::LED_CHIPSET_TABLE[chip_id].name : "Unknown";
    };

    auto get_color_order_name = [](uint8_t idx) -> const char* {
        static constexpr const char* NAMES[] = { "RGB", "RBG", "GRB", "GBR", "BRG", "BGR" };
        const size_t count = sizeof(NAMES) / sizeof(NAMES[0]);
        return (idx < count) ? NAMES[idx] : "Unknown";
    };

    // 3. Unified line processing (handles both 0 lines and N lines dynamically)
    uint32_t total_physical_leds = 0;
    uint32_t total_power_mw = 0;
    const uint16_t loop_count = (configured_lines == 0) ? 1 : configured_lines;

    std::stringstream hw_lines_stream;
    std::stringstream pwr_lines_stream;

    for (uint16_t i = 0; i < loop_count; ++i) {
        uint16_t line_len = num_led; // Default for 0 configured lines

        if (configured_lines > 0) {
            line_len = controller.nvs.read_uint16(nvs_key, "cfg_l_" + std::to_string(i) + "_cnt", 0);
            hw_lines_stream << "    Line " << (i + 1) << " Length:    " << line_len << "\n";
        } else {
            hw_lines_stream << "    Length:           " << line_len << "\n";
        }

        total_physical_leds += line_len;

        uint32_t line_power_mw = 0;
        if (is_on && line_len > 0) {
            const uint16_t powered_len = (line_len < signal_length) ? line_len : signal_length;
            line_power_mw = calculate_unscaled_power_mW(leds, powered_len);
        }
        total_power_mw += line_power_mw;

        const float power_w = line_power_mw / 1000.0f;
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
       << "    FPS:              " << (fps_counter * 1000 / (millis() + 1)) << "\n"
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
// there is room for optimization here to streamline unnecessary rgb-hsv conversions
void LedStrip::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(LedStrip, "-> set_rgb(%u, %u, %u)\n", new_rgb[0], new_rgb[1], new_rgb[2]);

    update_nvs_color_params(new_rgb, true); // true = is_rgb
    mode_controller->set_rgb(new_rgb);

    DBG_PRINTLN(LedStrip, "<- set_rgb()");
}

void LedStrip::set_r(const uint8_t r) {
    DBG_PRINTF(LedStrip, "-> set_r(%u)\n", r);
    set_rgb({r, mode_controller->get_rgb()[1], mode_controller->get_rgb()[2]});
    DBG_PRINTLN(LedStrip, "<- set_r()");
}

void LedStrip::set_g(const uint8_t g) {
    DBG_PRINTF(LedStrip, "-> set_g(%u)\n", g);
    set_rgb({mode_controller->get_rgb()[0], g, mode_controller->get_rgb()[2]});
    DBG_PRINTLN(LedStrip, "<- set_g()");
}

void LedStrip::set_b(const uint8_t b) {
    DBG_PRINTF(LedStrip, "-> set_b(%u)\n", b);
    set_rgb({mode_controller->get_rgb()[0], mode_controller->get_rgb()[1], b});
    DBG_PRINTLN(LedStrip, "<- set_b()");
}

void LedStrip::set_hsv(const std::array<uint8_t, 3> new_hsv) {
    DBG_PRINTF(LedStrip, "-> set_hsv(%u, %u, %u)\n", new_hsv[0], new_hsv[1], new_hsv[2]);
    
    update_nvs_color_params(new_hsv, false); // false = not rgb (is hsv)
    mode_controller->set_hsv(new_hsv);

    DBG_PRINTLN(LedStrip, "<- set_hsv()");
}

void LedStrip::set_h(const uint8_t h) {
    DBG_PRINTF(LedStrip, "-> set_h(%u)\n", h);
    array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({h, old_hsv[1], old_hsv[2]});
    DBG_PRINTLN(LedStrip, "<- set_h()");
}

void LedStrip::set_s(const uint8_t s) {
    DBG_PRINTF(LedStrip, "-> set_s(%u)\n", s);
    array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({old_hsv[0], s, old_hsv[2]});
    DBG_PRINTLN(LedStrip, "<- set_s()");
}

void LedStrip::set_v(const uint8_t v) {
    DBG_PRINTF(LedStrip, "-> set_v(%u)\n", v);
    array<uint8_t, 3> old_hsv = get_hsv();
    set_hsv({old_hsv[0], old_hsv[1], v});
    DBG_PRINTLN(LedStrip, "<- set_v()");
}

array<uint8_t, 3> LedStrip::get_rgb() const {

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

array<uint8_t, 3> LedStrip::get_hsv() const {

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

void LedStrip::adj_rgb(const array<int, 3> rgb_delta) {
    array<uint8_t, 3> adjusted_rgb = get_rgb();

    for(int i = 0; i < 3; i++) {
        int adj_value = adjusted_rgb[i] + rgb_delta[i];
        adjusted_rgb[i] = static_cast<uint8_t>(clamp<int>(adj_value, 0, 255));
    }

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

void LedStrip::adj_hsv(const array<int, 3> hsv_delta) {
    array<uint8_t, 3> adjusted_hsv = get_hsv();

    for(int i = 0; i < 3; i++) {
        int adj_value = adjusted_hsv[i] + hsv_delta[i];

        if (i == 0) { // Hue wraps around 0-255
            while(adj_value < 0) adj_value += 256;
            adjusted_hsv[i] = static_cast<uint8_t>(adj_value % 256);
        } else { // Saturation and Value constrain 0-255
            adjusted_hsv[i] = static_cast<uint8_t>(clamp<int>(adj_value, 0, 255));
        }
    }

    set_hsv(adjusted_hsv);
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

// =============================================================================
// Custom Methods: Brightness
// =============================================================================
void LedStrip::set_brightness(const uint8_t new_brightness) {
    DBG_PRINTF(LedStrip, "-> set_brightness(%u)\n", new_brightness);
    brightness->set_brightness(new_brightness);
    DBG_PRINTLN(LedStrip, "<- set_brightness()");
}

uint8_t LedStrip::get_brightness() const {

    return brightness->get_last_brightness();
}

void LedStrip::adj_brightness(const int brightness_delta) {
    int new_brightness = brightness->get_target_value() + brightness_delta;
    set_brightness(static_cast<uint8_t>(clamp<int>(new_brightness, 0, 255)));
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

    return brightness->get_state();
}

// =============================================================================
// Custom Methods: Mode
// =============================================================================
void LedStrip::set_mode(const uint8_t new_mode) {
    DBG_PRINTF(LedStrip, "-> set_mode(mode: %u)\n", new_mode);

    ModeConfig default_config = mode_controller->get_mode_config(new_mode);
    std::map<std::string, uint16_t> loaded_params;

    for (const auto& param : default_config.params) {
        std::string nvs_param_key = "m:" + std::to_string(new_mode) + ":" + param.key;
        uint16_t stored_val = controller.nvs.read_uint16(this->nvs_key, nvs_param_key, param.default_value);
        loaded_params[param.key] = stored_val;

        // Log each parameter as it is pulled from memory
        DBG_PRINTF(LedStrip, "   - Loaded Param [%s]: %u\n", param.key.c_str(), stored_val);
    }

    mode_controller->set_mode(new_mode, loaded_params);

    DBG_PRINTLN(LedStrip, "<- set_mode()");
}

void LedStrip::set_mode_param(std::string_view key, const uint16_t value) {
    // Note: %.*s is used for string_view because it isn't guaranteed to be null-terminated
    DBG_PRINTF(LedStrip, "-> set_mode_param(key: %.*s, val: %u)\n", (int)key.length(), key.data(), value);

    bool result = mode_controller->set_mode_param(key, value);

    if (result) {
        std::string nvs_param_key = "m:" + std::to_string(get_current_mode_id()) + ":" + std::string(key);
        controller.nvs.write_uint16(this->nvs_key, nvs_param_key, value);
        DBG_PRINTLN(LedStrip, "   Param applied and saved to NVS");

        // Directly call the web interface to push the update via WebSocket
        controller.web_interface.sync_param(key, value);

    } else {
        DBG_PRINTLN(LedStrip, "   ! Failed to set param: Key not found in current mode");
    }

    DBG_PRINTLN(LedStrip, "<- set_mode_param()");
}

void LedStrip::adj_mode_param(string_view key, const long value_delta) {
    for (const auto& param : this->mode_controller->get_current_mode_params()) {
        if (param.key == key) {
            long current_value = this->get_current_mode_param(std::string(key));
            long new_value = current_value + value_delta;
            uint16_t final_val;

            if (key == "hue") {
                final_val = static_cast<uint16_t>((new_value % 256 + 256) % 256);
            } else {
                final_val = static_cast<uint16_t>(std::clamp<long>(new_value, param.min_value, param.max_value));
            }

            this->set_mode_param(std::string(key), final_val);
            return;
        }
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
    DBG_PRINTLN(LedStrip, "-> reset_current_mode()");

    const uint8_t current_mode = get_current_mode_id();
    const ModeConfig default_config = mode_controller->get_mode_config(current_mode);

    for (const auto& param : default_config.params) {
        const std::string nvs_param_key =
            "m:" + std::to_string(current_mode) + ":" + param.key;

        controller.nvs.write_uint16(this->nvs_key, nvs_param_key, param.default_value);

//        if (param.key == "hue") {
//            set_h(static_cast<uint8_t>(param.default_value));
//        } else if (param.key == "sat") {
//            set_s(static_cast<uint8_t>(param.default_value));
//        } else if (param.key == "r") {
//            set_r(static_cast<uint8_t>(param.default_value));
//        } else if (param.key == "g") {
//            set_g(static_cast<uint8_t>(param.default_value));
//        } else if (param.key == "b") {
//            set_b(static_cast<uint8_t>(param.default_value));
//        } else {
//            set_mode_param(param.key, param.default_value);
//        }

        DBG_PRINTF(
            LedStrip,
            "   - Reset Param [%s] to default [%u] in NVS\n",
            param.key.c_str(),
            param.default_value
        );
    }

    set_mode(current_mode);

    DBG_PRINTLN(LedStrip, "<- reset_current_mode()");
}

std::string LedStrip::get_all_modes_json() const {
    DBG_PRINTLN(LedStrip, "get_all_modes()");
    return mode_controller->get_all_modes_json();
}

void LedStrip::adj_mode(const int mode_delta) {
    int new_mode = get_current_mode_id() + mode_delta;
    set_mode(static_cast<uint8_t>(clamp<int>(new_mode, 0, 255)));
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
    mode_controller->set_length(length);
    DBG_PRINTLN(LedStrip, "<- set_length()");
}

uint16_t LedStrip::get_length() const {

    return num_led;
}

// =============================================================================
// Custom Methods: Fill
// =============================================================================

void LedStrip::set_pixel(uint16_t i, std::array<uint8_t, 3> color_rgb) {
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

void LedStrip::set_all(const uint8_t r, const uint8_t g, const uint8_t b) {
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

void LedStrip::update_nvs_color_params(const std::array<uint8_t, 3> new_color, bool is_rgb) {
    std::array<uint8_t, 3> rgb_vals;
    std::array<uint8_t, 3> hsv_vals;

    if (is_rgb) {
        rgb_vals = new_color;
        hsv_vals = rgb_to_hsv(new_color);
        mode_controller->set_rgb(new_color);
    } else {
        hsv_vals = new_color;
        rgb_vals = hsv_to_rgb(new_color);
        mode_controller->set_hsv(new_color);
    }

    auto update_and_save = [&](const char* param_name, uint16_t value, bool force_write = false) {
        const bool applied = mode_controller->set_mode_param(param_name, value);

        if (applied || force_write) {
            const std::string nvs_param_key =
                "m:" + std::to_string(mode_controller->get_current_mode_id()) + ":" + param_name;

            controller.nvs.write_uint8(this->nvs_key, nvs_param_key, value);
        }
    };

    update_and_save("r",   rgb_vals[0], true); // force rgb write as it is global param
    update_and_save("g",   rgb_vals[1], true);
    update_and_save("b",   rgb_vals[2], true);
    update_and_save("hue", hsv_vals[0], false);
    update_and_save("sat", hsv_vals[1], false);
    update_and_save("val", hsv_vals[2], false);
}

bool LedStrip::set_leds_chipset(const LedStrip::LEDChipset chipset) {
    switch (chipset) {
        case LEDChipset::APA102:          FastLED.addLeds<APA102, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA102HD:        FastLED.addLeds<APA102HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA104:          FastLED.addLeds<APA104, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::APA106:          FastLED.addLeds<APA106, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::DOTSTAR:         FastLED.addLeds<DOTSTAR, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::DOTSTARHD:       FastLED.addLeds<DOTSTARHD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GE8822:          FastLED.addLeds<GE8822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GS1903:          FastLED.addLeds<GS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GW6205:          FastLED.addLeds<GW6205, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::GW6205_400KHZ:   FastLED.addLeds<GW6205_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::HD107:           FastLED.addLeds<HD107, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::HD107HD:         FastLED.addLeds<HD107HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD1886:         FastLED.addLeds<LPD1886, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD1886_8BIT:    FastLED.addLeds<LPD1886_8BIT, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD6803:         FastLED.addLeds<LPD6803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::LPD8806:         FastLED.addLeds<LPD8806, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::NEOPIXEL:        FastLED.addLeds<NEOPIXEL, LED_PIN_DATA>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::P9813:           FastLED.addLeds<P9813, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::PL9823:          FastLED.addLeds<PL9823, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK6812:          FastLED.addLeds<SK6812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK6822:          FastLED.addLeds<SK6822, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK9822:          FastLED.addLeds<SK9822, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SK9822HD:        FastLED.addLeds<SK9822HD, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16703:         FastLED.addLeds<SM16703, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16716:         FastLED.addLeds<SM16716, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::SM16824E:        FastLED.addLeds<SM16824E, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1803:          FastLED.addLeds<TM1803, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1804:          FastLED.addLeds<TM1804, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1809:          FastLED.addLeds<TM1809, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1812:          FastLED.addLeds<TM1812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::TM1829:          FastLED.addLeds<TM1829, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1903:         FastLED.addLeds<UCS1903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1903B:        FastLED.addLeds<UCS1903B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1904:         FastLED.addLeds<UCS1904, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS1912:         FastLED.addLeds<UCS1912, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::UCS2903:         FastLED.addLeds<UCS2903, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2801:          FastLED.addLeds<WS2801, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2803:          FastLED.addLeds<WS2803, LED_PIN_DATA, LED_PIN_CLOCK, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2811:          FastLED.addLeds<WS2811, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2811_400KHZ:   FastLED.addLeds<WS2811_400, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2812:          FastLED.addLeds<WS2812, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2812B:         FastLED.addLeds<WS2812B, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2813:          FastLED.addLeds<WS2813, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2815:          FastLED.addLeds<WS2815, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2816:          FastLED.addLeds<WS2816, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;
        case LEDChipset::WS2852:          FastLED.addLeds<WS2852, LED_PIN_DATA, RGB>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection(TypicalLEDStrip); return true;

        default: return false;
    }
}
