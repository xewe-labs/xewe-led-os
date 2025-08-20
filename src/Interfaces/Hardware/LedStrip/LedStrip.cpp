#include "LedStrip.h"
#include "../../../SystemController/SystemController.h"

LedStrip::LedStrip(SystemController& controller_ref)
    : Interface(controller_ref, "led", "led", false, true),
      num_led(0),
      led_mode_mutex(NULL),
      led_data_mutex(NULL)
    {
        DBG_PRINTLN(LedStrip, "-> LedStrip::LedStrip()");
        commands_storage.push_back({
            "set_rgb",
            "Set RGB color",
            std::string("Sample Use: $") + lower(module_name) + " set_rgb 255 0 0",
            3,
            [this](std::string args){ set_rgb_cli(args); }
        });
        commands_storage.push_back({
            "set_r",
            "Set red channel",
            std::string("Sample Use: $") + lower(module_name) + " set_r 127",
            1,
            [this](std::string args){ set_r_cli(args); }
        });
        commands_storage.push_back({
            "set_g",
            "Set green channel",
            std::string("Sample Use: $") + lower(module_name) + " set_g 255",
            1,
            [this](std::string args){ set_g_cli(args); }
        });
        commands_storage.push_back({
            "set_b",
            "Set blue channel",
            std::string("Sample Use: $") + lower(module_name) + " set_b 200",
            1,
            [this](std::string args){ set_b_cli(args); }
        });
        commands_storage.push_back({
            "set_hsv",
            "Set HSV color",
            std::string("Sample Use: $") + lower(module_name) + " set_hsv 75 255 0",
            3,
            [this](std::string args){ set_hsv_cli(args); }
        });
        commands_storage.push_back({
            "set_hue",
            "Set hue channel",
            std::string("Sample Use: $") + lower(module_name) + " set_hue 255",
            1,
            [this](std::string args){ set_hue_cli(args); }
        });
        commands_storage.push_back({
            "set_sat",
            "Set saturation channel",
            std::string("Sample Use: $") + lower(module_name) + " set_sat 0",
            1,
            [this](std::string args){ set_sat_cli(args); }
        });
        commands_storage.push_back({
            "set_val",
            "Set value channel",
            std::string("Sample Use: $") + lower(module_name) + " set_val 255",
            1,
            [this](std::string args){ set_val_cli(args); }
        });
        commands_storage.push_back({
            "set_brightness",
            "Set global brightness",
            std::string("Sample Use: $") + lower(module_name) + " set_brightness 255",
            1,
            [this](std::string args){ set_brightness_cli(args); }
        });
        commands_storage.push_back({
            "set_state",
            "Set on/off state",
            std::string("Sample Use: $") + lower(module_name) + " set_state 0",
            1,
            [this](std::string args){ set_state_cli(args); }
        });
        commands_storage.push_back({
            "toggle_state",
            "If off->on, if on->off",
            std::string("Sample Use: $") + lower(module_name) + " toggle_state",
            0,
            [this](std::string){ toggle_state_cli(); }
        });
        commands_storage.push_back({
            "turn_on",
            "Turn strip on",
            std::string("Sample Use: $") + lower(module_name) + " turn_on",
            0,
            [this](std::string){ turn_on_cli(); }
        });
        commands_storage.push_back({
            "turn_off",
            "Turn strip off",
            std::string("Sample Use: $") + lower(module_name) + " turn_off",
            0,
            [this](std::string){ turn_off_cli(); }
        });
        commands_storage.push_back({
            "set_mode",
            "Set LED strip mode",
            std::string("Sample Use: $") + lower(module_name) + " set_mode 0",
            1,
            [this](std::string args){ set_mode_cli(args); }
        });
        commands_storage.push_back({
            "set_length",
            "Set new number of LEDs",
            std::string("Sample Use: $") + lower(module_name) + " set_length 500",
            1,
            [this](std::string args){ set_length_cli(args); }
        });
        DBG_PRINTLN(LedStrip, "<- LedStrip::LedStrip()");
    }

LedStrip::~LedStrip() {
    DBG_PRINTLN(LedStrip, "-> LedStrip::~LedStrip()");
    if (led_mode_mutex != NULL) {
        vSemaphoreDelete(led_mode_mutex);
        led_mode_mutex = NULL;
    }
    if (led_data_mutex != NULL) {
        vSemaphoreDelete(led_data_mutex);
        led_data_mutex = NULL;
    }
    DBG_PRINTLN(LedStrip, "LedStrip: Destructor called, mutexes deleted");
    DBG_PRINTLN(LedStrip, "<- LedStrip::~LedStrip()");
}

void LedStrip::begin(const ModuleConfig& cfg) {
    const auto& config = static_cast<const LedStripConfig&>(cfg);
    DBG_PRINTLN(LedStrip, "LedStrip: begin() called");

//    this->leds                   = config.leds                  ;
    this->num_led                = config.num_led               ;
    this->color_transition_delay = config.color_transition_delay;

    FastLED.addLeds<LED_STRIP_TYPE, PIN_LED_STRIP, LED_STRIP_COLOR_ORDER>(leds, LED_STRIP_NUM_LEDS_MAX).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(255);

//    if (this->leds == nullptr) {
//        DBG_PRINTLN(LedStrip, "FATAL ERROR: leds_ptr provided to begin() is null!");
//        DBG_PRINTLN(LedStrip, "<- LedStrip::begin()");
//        return;
//    }

    frame_timer = std::make_unique<AsyncTimer<uint8_t>>(config.led_controller_frame_delay);
    brightness = std::make_unique<Brightness>(config.brightness_transition_delay, 0, 0);
    led_mode = std::make_unique<ColorSolid>(this, 0, 0, 0);

    led_mode_mutex = xSemaphoreCreateMutex();
    if (led_mode_mutex == NULL) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: LedStrip led_mode_mutex could not be created!");
    }

    led_data_mutex = xSemaphoreCreateMutex();
    if (led_data_mutex == NULL) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: LedStrip led_data_mutex could not be created!");
    }

    frame_timer->initiate();
    DBG_PRINTLN(LedStrip, "<- LedStrip::begin()");
}

void LedStrip::loop() {
//    DBG_PRINTLN(LedStrip, "-> LedStrip::loop()");
    if (frame_timer->is_active()) {
        DBG_PRINTLN(LedStrip, "<- LedStrip::loop() (frame_timer active)");
        return;
    }
    frame_timer->reset();
    frame_timer->initiate();

    std::array<uint8_t, 3> color_to_fill = {0, 0, 0}; // Default to black
    bool needs_mode_reassignment = false;
    uint8_t current_mode_id_local = MODE_SOLID; // Default to a known mode ID
    std::array<uint8_t, 3> rgb_temp_for_reassign = {0, 0, 0};

    if (xSemaphoreTake(led_mode_mutex, (TickType_t)10) == pdTRUE) { // Use a short timeout
        if (led_mode) {
            // CRITICAL ASSUMPTION: led_mode->frame() now ONLY updates the LedMode's internal state
            // and does NOT call LedStrip::fill_all() or other LedStrip methods that might lock.
            led_mode->loop();

            current_mode_id_local = led_mode->get_mode_id();
            color_to_fill = led_mode->get_rgb(); // Get current color from the mode

            if (current_mode_id_local == MODE_CHANGING) { // Check if it's ColorChanging
                if (led_mode->is_done()) {
                    needs_mode_reassignment = true;
                    rgb_temp_for_reassign = led_mode->get_rgb(); // This is the target color of ColorChanging
                }
            }

            if (needs_mode_reassignment) {
                // Transition from ColorChanging (MODE_CHANGING) to ColorSolid (MODE_SOLID)
                led_mode = std::make_unique<ColorSolid>(this, rgb_temp_for_reassign[0], rgb_temp_for_reassign[1], rgb_temp_for_reassign[2]);
                // After reassignment, get the new solid color for the current frame
                color_to_fill = led_mode->get_rgb();
            }
        } else {
            DBG_PRINTLN(LedStrip, "Warning: led_mode is nullptr in loop()");
        }
        xSemaphoreGive(led_mode_mutex);

        // Now call fill_all, which will acquire its own led_data_mutex
        this->fill_all(color_to_fill);

    } else {
        DBG_PRINTLN(LedStrip, "Warning: Could not take led_mode_mutex in loop(), skipping led_mode logic.");
        // Optionally, draw a default color or last known safe color
        // this->fill_all(0,0,0); // Example: turn off if mode can't be processed
    }
//    DBG_PRINTLN(LedStrip, "<- LedStrip::loop()");
}

void LedStrip::reset(bool verbose) {
    // todo
}

void LedStrip::sync_color(std::array<uint8_t,3> color) {
    set_rgb(color);
}

void LedStrip::sync_brightness(uint8_t brightness) {
    set_brightness(brightness);
}

void LedStrip::sync_state(uint8_t state) {
    set_state(state);
}

void LedStrip::sync_mode(uint8_t mode) {
    set_mode(mode);
}

void LedStrip::sync_length(uint16_t length) {
    set_length(length);
}

void LedStrip::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length)
{
    sync_color(color);
    sync_brightness(brightness);
    sync_state(state);
    sync_mode(mode);
    sync_length(length);
}

std::string LedStrip::status(bool verbose) const {
    DBG_PRINTLN(LedStrip, "status()");

    // Use a stringstream to efficiently build the multi-line status string.
    std::stringstream status_stream;

    status_stream << "--- LED Strip Status ---\n"
                  << "Hardware Config (firmware):\n"
                  << "  - Pin:          GPIO" << static_cast<int>(PIN_LED_STRIP) << "\n"
                  << "  - Type:         " << TO_STRING(LED_STRIP_TYPE) << "\n"
                  << "  - Color Order:  " << TO_STRING(LED_STRIP_COLOR_ORDER) << "\n"
                  << "  - Max LEDs:     " << LED_STRIP_NUM_LEDS_MAX << "\n"
                  << "\n"
                  << "Live State:\n"
                  << "  - Length:       " << get_length() << "\n"
                  << "  - State:        " << (get_state() ? "ON" : "OFF") << "\n"
                  << "  - Brightness:   " << static_cast<int>(get_brightness()) << "\n"
                  << "  - Mode:         " << get_mode_name().c_str() << "\n"
                  << "  - Color (RGB):  ("
                  << static_cast<int>(get_r()) << ", "
                  << static_cast<int>(get_g()) << ", "
                  << static_cast<int>(get_b()) << ")\n"
                  << "------------------------\n";

    // Convert the stringstream to a std::string
    std::string status_string = status_stream.str();

    // If verbose is requested, print the generated string to the serial port
    if (verbose) {
        // The .c_str() method provides compatibility with serial.print()
        controller.serial_port.print(status_string.c_str());
    }

    return status_string;
}

void LedStrip::set_mode(uint8_t new_mode_id) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_mode(new_mode_id: %u)\n", new_mode_id);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTF(LedStrip, "set_mode: %u\n", new_mode_id);
        // IMPORTANT: Ensure the current LedMode's state (e.g. current color) is retrieved
        // if needed for a smooth transition to the new mode, before reassigning led_mode.
        std::array<uint8_t, 3> current_rgb = {0,0,0};
        if(led_mode) {
            current_rgb = led_mode->get_rgb();
        }

        switch (static_cast<LedModeID>(new_mode_id)) {
            case MODE_SOLID:
                // When switching to solid, use the current color of the previous mode
                led_mode = std::make_unique<ColorSolid>(this, current_rgb[0], current_rgb[1], current_rgb[2]);
                break;
            // Add cases for other modes, e.g.,
            // case MODE_CHANGING:
            //     // Requires a target color; perhaps set_rgb should be used for this.
            //     // Or, this set_mode could transition to a default ColorChanging effect.
            //     led_mode = std::make_unique<ColorChanging>(this, current_rgb[0], current_rgb[1], current_rgb[2], /*target r*/255, /*target g*/0, /*target b*/0, 'r', color_transition_delay);
            //     break;
            default:
                DBG_PRINTF(LedStrip, "set_mode: Unknown mode ID %u\n", new_mode_id);
                // Optionally, revert to a default mode or do nothing
                led_mode = std::make_unique<ColorSolid>(this, current_rgb[0], current_rgb[1], current_rgb[2]); // Revert to solid with current color
                break;
        }
        xSemaphoreGive(led_mode_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_mode");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_mode()");
}

void LedStrip::set_rgb(std::array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_rgb(new_rgb: {%u, %u, %u})\n", new_rgb[0], new_rgb[1], new_rgb[2]);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTF(LedStrip, "set_rgb: R=%d G=%d B=%d\n", new_rgb[0], new_rgb[1], new_rgb[2]);
        std::array<uint8_t, 3> old_rgb = {0,0,0};
        if (led_mode) {
            old_rgb = led_mode->get_rgb(); // Get current color of the current mode
        }

        // Check if the *target* is already the current *display* color of a potentially static mode
        // Or if the current mode is already transitioning to this target.
        bool already_set = false;
        if (led_mode) {
            if (led_mode->get_mode_id() == MODE_SOLID && old_rgb == new_rgb) {
                already_set = true;
            } else if (led_mode->get_mode_id() == MODE_CHANGING) {
                std::array<uint8_t, 3> target_rgb = led_mode->get_target_rgb();
                if (target_rgb == new_rgb) {
                    already_set = true;
                }
            }
        }

        if (already_set) {
            DBG_PRINTLN(LedStrip, "Color or target color already set");
            xSemaphoreGive(led_mode_mutex);
            DBG_PRINTLN(LedStrip, "<- LedStrip::set_rgb() (already set)");
            return;
        }

        // Start transition from the current actual color (old_rgb)
        led_mode = std::make_unique<ColorChanging>(
            this,
            old_rgb[0], old_rgb[1], old_rgb[2], // Start color
            new_rgb[0], new_rgb[1], new_rgb[2],                          // Target color
            'r',                              // RGB mode for ColorChanging
            color_transition_delay);

        xSemaphoreGive(led_mode_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_rgb");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_rgb()");
}

void LedStrip::set_r(uint8_t r_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_r(r_val: %u)\n", r_val);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_g = 0;
        uint8_t current_b = 0;
        if (led_mode) {
            current_g = led_mode->get_g();
            current_b = led_mode->get_b();
        }
        xSemaphoreGive(led_mode_mutex);
        set_rgb({r_val, current_g, current_b});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_r");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_r()");
}

void LedStrip::set_g(uint8_t g_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_g(g_val: %u)\n", g_val);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_r = 0;
        uint8_t current_b = 0;
        if (led_mode) {
            current_r = led_mode->get_r();
            current_b = led_mode->get_b();
        }
        xSemaphoreGive(led_mode_mutex);
        set_rgb({current_r, g_val, current_b});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_g");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_g()");
}

void LedStrip::set_b(uint8_t b_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_b(b_val: %u)\n", b_val);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_r = 0;
        uint8_t current_g = 0;
        if (led_mode) {
            current_r = led_mode->get_r();
            current_g = led_mode->get_g();
        }
        xSemaphoreGive(led_mode_mutex);
        set_rgb({current_r, current_g, b_val});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_b");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_b()");
}

void LedStrip::set_hsv(std::array<uint8_t, 3> new_hsv) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_hsv(new_hsv: {%u, %u, %u})\n", new_hsv[0], new_hsv[1], new_hsv[2]);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTF(LedStrip, "set_hsv: H=%d S=%d V=%d\n", new_hsv[0], new_hsv[1], new_hsv[2]);

        std::array<uint8_t, 3> current_hsv_val = {0,0,0};
        std::array<uint8_t, 3> current_rgb_for_transition = {0,0,0};
        bool already_set = false;

        if (led_mode) {
            current_hsv_val = led_mode->get_hsv();
            current_rgb_for_transition = led_mode->get_rgb();

            if (led_mode->get_mode_id() == MODE_SOLID && current_hsv_val == new_hsv) {
                 already_set = true;
            } else if (led_mode->get_mode_id() == MODE_CHANGING) {
                std::array<uint8_t, 3> target_hsv = led_mode->get_target_hsv();
                if (target_hsv == new_hsv) {
                    already_set = true;
                }
            }
        }

        if (already_set) {
            DBG_PRINTLN(LedStrip, "HSV Color or target HSV color already set");
            xSemaphoreGive(led_mode_mutex);
            DBG_PRINTLN(LedStrip, "<- LedStrip::set_hsv() (already set)");
            return;
        }

        led_mode = std::make_unique<ColorChanging>(
            this,
            current_rgb_for_transition[0], current_rgb_for_transition[1], current_rgb_for_transition[2],
            new_hsv[0], new_hsv[1], new_hsv[2],
            'h', // HSV mode for ColorChanging
            color_transition_delay);

        xSemaphoreGive(led_mode_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_hsv");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_hsv()");
}

void LedStrip::set_h(uint8_t h_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_h(h_val: %u)\n", h_val);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_s = 0;
        uint8_t current_v = 0;
        if (led_mode) {
            current_s = led_mode->get_s();
            current_v = led_mode->get_v();
        }
        xSemaphoreGive(led_mode_mutex);
        set_hsv({h_val, current_s, current_v});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_h");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_h()");
}

void LedStrip::set_s(uint8_t s_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_s(s_val: %u)\n", s_val);
     if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_h = 0;
        uint8_t current_v = 0;
        if (led_mode) {
            current_h = led_mode->get_h();
            current_v = led_mode->get_v();
        }
        xSemaphoreGive(led_mode_mutex);
        set_hsv({current_h, s_val, current_v});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_s");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_s()");
}

void LedStrip::set_v(uint8_t v_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_v(v_val: %u)\n", v_val);
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_h = 0;
        uint8_t current_s = 0;
        if (led_mode) {
            current_h = led_mode->get_h();
            current_s = led_mode->get_s();
        }
        xSemaphoreGive(led_mode_mutex);
        set_hsv({current_h, current_s, v_val});
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_v");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_v()");
}

void LedStrip::set_brightness(uint8_t new_brightness) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_brightness(new_brightness: %u)\n", new_brightness);
    if (brightness) {
        brightness->set_brightness(new_brightness);
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_brightness()");
}

void LedStrip::set_state(uint8_t state_val) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_state(state_val: %u)\n", state_val);
    if (state_val) {
        turn_on();
    } else {
        turn_off();
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_state()");
}

void LedStrip::toggle_state() {
    if(brightness->get_state()) {
        turn_off();
    } else {
        turn_on();
    }
}

void LedStrip::turn_on() {
    DBG_PRINTLN(LedStrip, "-> LedStrip::turn_on()");
    if (brightness) {
        brightness->turn_on();
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::turn_on()");
}

void LedStrip::turn_off() {
    DBG_PRINTLN(LedStrip, "-> LedStrip::turn_off()");
    if (brightness) {
        brightness->turn_off();
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::turn_off()");
}

void LedStrip::fill_all(std::array<uint8_t, 3> color_rgb) {
//    DBG_PRINTF(LedStrip, "-> LedStrip::fill_all(color_rgb: {%u, %u, %u})\n", color_rgb[0], color_rgb[1], color_rgb[2]);
    if (xSemaphoreTake(led_data_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint16_t i = 0; i < num_led; i++) {
            set_pixel(i, color_rgb);
        }
        if (num_led > 0) {
            FastLED.show();
        }
        xSemaphoreGive(led_data_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_data_mutex in fill_all");
    }
//    DBG_PRINTLN(LedStrip, "<- LedStrip::fill_all()");
}

void LedStrip::set_pixel (uint16_t i, std::array<uint8_t, 3> color_rgb) {
//    DBG_PRINTF(LedStrip, "-> LedStrip::set_pixel(i: %u, color_rgb: {%u, %u, %u})\n", i, color_rgb[0], color_rgb[1], color_rgb[2]);
    if (leds && i < num_led) {
        if (brightness) {
            std::array<uint8_t, 3> dimmed_color = brightness->get_dimmed_color(color_rgb);
            leds[i] = CRGB(dimmed_color[0], dimmed_color[1], dimmed_color[2]);
        } else {
            leds[i] = CRGB(color_rgb[0], color_rgb[1], color_rgb[2]);
        }
    }
//    DBG_PRINTLN(LedStrip, "<- LedStrip::set_pixel()");
}

void LedStrip::set_length(uint16_t new_length) {
    DBG_PRINTF(LedStrip, "-> LedStrip::set_length(new_length: %u)\n", new_length);
    if (xSemaphoreTake(led_data_mutex, portMAX_DELAY) == pdTRUE) {
        if (leds) {
            for (uint16_t i = 0; i < num_led; i++) {
                leds[i] = CRGB::Black;
            }
            if (num_led > 0) {
                FastLED.show();
            }
        }
        num_led = new_length;
        DBG_PRINTF(LedStrip, "Set num_led to %u\n", num_led);
        xSemaphoreGive(led_data_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_data_mutex in set_length");
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_length()");
}

uint16_t LedStrip::get_length() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_length()");
    DBG_PRINTF(LedStrip, "<- LedStrip::get_length() returns: %u\n", num_led);
    return num_led;
}

std::array<uint8_t,3> LedStrip::get_rgb() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_rgb()");
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_rgb();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_rgb"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_rgb() returns: {%u, %u, %u}\n", res[0], res[1], res[2]);
    return res;
}

uint8_t LedStrip::get_r() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_r()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_r();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_r"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_r() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_g() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_g()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_g();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_g"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_g() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_b() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_b()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_b();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_b"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_b() returns: %u\n", res);
    return res;
}

std::array<uint8_t,3> LedStrip::get_target_rgb() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_rgb()");
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_rgb();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_rgb"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_rgb() returns: {%u, %u, %u}\n", res[0], res[1], res[2]);
    return res;
}

uint8_t LedStrip::get_target_r() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_r()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_r();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_r"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_r() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_target_g() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_g()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_g();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_g"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_g() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_target_b() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_b()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_b();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_b"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_b() returns: %u\n", res);
    return res;
}

std::array<uint8_t, 3> LedStrip::get_hsv() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_hsv()");
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_hsv();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_hsv"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_hsv() returns: {%u, %u, %u}\n", res[0], res[1], res[2]);
    return res;
}

uint8_t LedStrip::get_h() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_h()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_h();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_h"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_h() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_s() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_s()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_s();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_s"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_s() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_v() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_v()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_v();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_v"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_v() returns: %u\n", res);
    return res;
}

std::array<uint8_t, 3> LedStrip::get_target_hsv() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_hsv()");
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_hsv();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_hsv"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_hsv() returns: {%u, %u, %u}\n", res[0], res[1], res[2]);
    return res;
}

uint8_t LedStrip::get_target_h() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_h()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_h();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_h"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_h() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_target_s() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_s()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_s();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_s"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_s() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_target_v() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_v()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_v();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_v"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_v() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_brightness() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_brightness()");
    uint8_t res = 0;
    if (brightness) {
        res = brightness->get_last_brightness();
    }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_brightness() returns: %u\n", res);
    return res;
}

bool LedStrip::get_state() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_state()");
    bool res = false;
    if (brightness) {
        res = brightness->get_state();
    }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_state() returns: %s\n", res ? "true" : "false");
    return res;
}

String LedStrip::get_mode_name() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_mode_name()");
    String res = "";
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_mode_name();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_mode_name"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_mode_name() returns: %s\n", res.c_str());
    return res;
}

uint8_t LedStrip::get_mode_id() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_mode_id()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_mode_id();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_mode_id"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_mode_id() returns: %u\n", res);
    return res;
}

String LedStrip::get_target_mode_name() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_mode_name()");
    String res = "";
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_mode_name();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_mode_name"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_mode_name() returns: %s\n", res.c_str());
    return res;
}

uint8_t LedStrip::get_target_mode_id() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_mode_id()");
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_mode_id();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_mode_id"); }
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_mode_id() returns: %u\n", res);
    return res;
}

uint8_t LedStrip::get_target_brightness() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_brightness()");
    uint8_t res = brightness->get_target_value();
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_brightness() returns: %u\n", res);
    return res;
}

bool LedStrip::get_target_state() const {
    DBG_PRINTLN(LedStrip, "-> LedStrip::get_target_state()");
    bool res = brightness->get_state();
    DBG_PRINTF(LedStrip, "<- LedStrip::get_target_state() returns: %s\n", res ? "true" : "false");
    return res;
}


void LedStrip::set_rgb_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    int i1 = args.indexOf(' ');
    if (i1 == -1) return;
    int i2 = args.indexOf(' ', i1 + 1);
    if (i2 == -1) return;

    uint8_t r = args.substring(0, i1).toInt();
    uint8_t g = args.substring(i1 + 1, i2).toInt();
    uint8_t b = args.substring(i2 + 1).toInt();

    std::array<uint8_t, 3> new_rgb = {r, g, b};
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_r_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());

    uint8_t r = args.toInt()
    uint8_t g = led_mode.rgb[1];
    uint8_t b = led_mode.rgb[2];

    std::array<uint8_t, 3> new_rgb = {r, g, b};
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_g_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());

    uint8_t r = led_mode.rgb[0];
    uint8_t g = args.toInt()
    uint8_t b = led_mode.rgb[2];

    std::array<uint8_t, 3> new_rgb = {r, g, b};
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_b_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());

    uint8_t r = led_mode.rgb[0];
    uint8_t g = led_mode.rgb[1];
    uint8_t b = args.toInt()

    std::array<uint8_t, 3> new_rgb = {r, g, b};
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_hsv_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    int i1 = args.indexOf(' ');
    if (i1 == -1) return;
    int i2 = args.indexOf(' ', i1 + 1);
    if (i2 == -1) return;

    uint8_t h = args.substring(0, i1).toInt();
    uint8_t s = args.substring(i1 + 1, i2).toInt();
    uint8_t v = args.substring(i2 + 1).toInt();

    std::array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_hue_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    std::array<uint8_t, 3> current_hsv = rgb_to_hsv(led_mode.rgb);

    uint8_t h = args.toInt()
    uint8_t s = current_hsv[1];
    uint8_t v = current_hsv[2];

    std::array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_sat_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    std::array<uint8_t, 3> current_hsv = rgb_to_hsv(led_mode.rgb);

    uint8_t h = current_hsv[0];
    uint8_t s = args.toInt()
    uint8_t v = current_hsv[2];

    std::array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_val_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    std::array<uint8_t, 3> current_hsv = rgb_to_hsv(led_mode.rgb);

    uint8_t h = current_hsv[0];
    uint8_t s = current_hsv[1];
    uint8_t v = args.toInt()

    std::array<uint8_t, 3> new_rgb = hsv_to_rgb({h, s, v});
    controller.sync_rgb({true. true, true, true, true}, new_rgb);
}

void LedStrip::set_brightness_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    controller.sync_brightness({true. true, true, true, true}, args.toInt());
}

void LedStrip::set_state_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    controller.sync_state({true. true, true, true, true}, args.toInt());
}

void LedStrip::toggle_state_cli() {
    controller.sync_state({true. true, true, true, true}, !brightness->get_state());
}

void LedStrip::turn_on_cli() {
    controller.sync_state({true. true, true, true, true}, 0);
}

void LedStrip::turn_off_cli() {
    controller.sync_state({true. true, true, true, true}, 1);
}

void LedStrip::set_mode_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    controller.sync_mode({true. true, true, true, true}, args.toInt());
}

void LedStrip::set_length_cli(std::string_view args_sv) {
    String args(args_sv.data(), args_sv.length());
    controller.sync_length({true. true, true, true, true}, args.toInt());
}


// Convert RGB → HSV
std::array<uint8_t, 3> LedMode::rgb_to_hsv(std::array<uint8_t, 3> input_rgb) {
    DBG_PRINTF(LedMode, "-> LedMode::rgb_to_hsv(input_rgb: {%u, %u, %u})\n", input_rgb[0], input_rgb[1], input_rgb[2]);

    float r = input_rgb[0] / 255.0f;
    float g = input_rgb[1] / 255.0f;
    float b = input_rgb[2] / 255.0f;

    float s = step(b, g);
    float px = mix(b, g, s);
    float py = mix(g, b, s);
    float pz = mix(-1.0f, 0.0f, s);
    float pw = mix(0.6666666f, -0.3333333f, s);
    s = step(px, r);
    float qx = mix(px, r, s);
    float qz = mix(pw, pz, s);
    float qw = mix(r, px, s);
    float d = qx - min(qw, py);
    float hue_float = abs(qz + (qw - py) / (6.0f * d + 1e-10f));
    float sat_float = d / (qx + 1e-10f);
    // hsv[2] = qx; not used for this lib

    std::array<uint8_t, 3> output_hsv = {(uint8_t)(hue_float * 255), (uint8_t)(sat_float * 255), (uint8_t)(qx * 255)};

    DBG_PRINTF(LedMode, "<- LedMode::rgb_to_hsv() returns: {%u, %u, %u}\n", output_hsv[0], output_hsv[1], output_hsv[2]);

    return output_hsv;
}

std::array<uint8_t, 3> LedMode::hsv_to_rgb(std::array<uint8_t, 3> input_hsv) {
    DBG_PRINTF(LedMode, "-> LedMode::hsv_to_rgb(input_hsv: {%u, %u, %u})\n", input_hsv[0], input_hsv[1], input_hsv[2]);

    float h_float = map(input_hsv[0], 0, 255, 0, 360);
    float s_float = map(input_hsv[1], 0, 255, 0, 100);
    float v_float = map(input_hsv[2], 0, 255, 0, 100);

    int i;
    float m, n, f;
    std::array<uint8_t, 3> rgb_temp = {0, 0, 0};
    s_float /= 100;
    v_float /= 100;

    if (s_float == 0) {
        rgb_temp[0] = rgb_temp[1] = rgb_temp[2] = round(v_float * 255);
        DBG_PRINTF(LedMode, "<- LedMode::hsv_to_rgb() returns (grayscale): {%u, %u, %u}\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
        return {rgb_temp[0], rgb_temp[1], rgb_temp[2]};
    }

    h_float /= 60;
    i = floor(h_float);
    f = h_float - i;

    if (!(i & 1)) {
        f = 1 - f;
    }

    m = v_float * (1 - s_float);
    n = v_float * (1 - s_float * f);

    switch (i) {
        case 0:
        case 6:
            rgb_temp[0] = round(v_float * 255);
            rgb_temp[1] = round(n * 255);
            rgb_temp[2] = round(m * 255);
            break;
        case 1:
            rgb_temp[0] = round(n * 255);
            rgb_temp[1] = round(v_float * 255);
            rgb_temp[2] = round(m * 255);
            break;
        case 2:
            rgb_temp[0] = round(m * 255);
            rgb_temp[1] = round(v_float * 255);
            rgb_temp[2] = round(n * 255);
            break;
        case 3:
            rgb_temp[0] = round(m * 255);
            rgb_temp[1] = round(n * 255);
            rgb_temp[2] = round(v_float * 255);
            break;
        case 4:
            rgb_temp[0] = round(n * 255);
            rgb_temp[1] = round(m * 255);
            rgb_temp[2] = round(v_float * 255);
            break;
        case 5:
            rgb_temp[0] = round(v_float * 255);
            rgb_temp[1] = round(m * 255);
            rgb_temp[2] = round(n * 255);
            break;
    }

    DBG_PRINTF(LedMode, "<- LedMode::hsv_to_rgb() returns: {%u, %u, %u}\n", rgb_temp[0], rgb_temp[1], rgb_temp[2]);
    return {rgb_temp[0], rgb_temp[1], rgb_temp[2]};
}

float LedMode::fract(float x) {
//    DBG_PRINTF(LedMode, "-> LedMode::fract(x: %f)\n", x);
    float result = x - int(x);
//    DBG_PRINTF(LedMode, "<- LedMode::fract() returns: %f\n", result);
    return result;
}

float LedMode::mix(float a, float b, float t) {
//    DBG_PRINTF(LedMode, "-> LedMode::mix(a: %f, b: %f, t: %f)\n", a, b, t);
    float result = a + (b - a) * t;
//    DBG_PRINTF(LedMode, "<- LedMode::mix() returns: %f\n", result);
    return result;
}

float LedMode::step(float e, float x) {
//    DBG_PRINTF(LedMode, "-> LedMode::step(e: %f, x: %f)\n", e, x);
    float result = x < e ? 0.0 : 1.0;
//    DBG_PRINTF(LedMode, "<- LedMode::step() returns: %f\n", result);
    return result;
}