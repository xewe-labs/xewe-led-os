#include "LedStrip.h"
// Assuming LedMode.h, ColorSolid.h, ColorChanging.h are included via LedStrip.h or directly as needed.
// Assuming Debug.h is available.

LedStrip::LedStrip()
    : leds(nullptr), // Initialize pointer to nullptr
      num_led(0),
      led_mode_mutex(NULL),
      led_data_mutex(NULL) {
    DBG_PRINTLN(LedStrip, "-> LedStrip::LedStrip()");
    DBG_PRINTLN(LedStrip, "LedStrip: Constructor called (empty)");
    DBG_PRINTLN(LedStrip, "<- LedStrip::LedStrip()");
}

// The begin() method now accepts the leds_ptr and handles all initialization.
void LedStrip::begin(CRGB* leds_ptr, uint16_t count) {
    DBG_PRINTF(LedStrip, "-> LedStrip::begin(leds_ptr: %p, count: %u)\n", (void*)leds_ptr, count);
    DBG_PRINTLN(LedStrip, "LedStrip: begin() called");

    // Assign the hardware pointer
    this->num_led = count;   // <--- FIX: Set the initial length
    this->leds = leds_ptr;
    if (this->leds == nullptr) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: leds_ptr provided to begin() is null!");
        // Handle this critical error, as the object is unusable without a valid LED array.
        DBG_PRINTLN(LedStrip, "<- LedStrip::begin()");
        return;
    }

    // Initialize smart pointers and timers
    frame_timer = std::make_unique<AsyncTimer<uint8_t>>(led_controller_frame_delay);
    brightness = std::make_unique<Brightness>(brightness_transition_delay, 0, 0);
    led_mode = std::make_unique<ColorSolid>(this, 0, 0, 0); // Default to solid black (off)

    // Create mutexes
    led_mode_mutex = xSemaphoreCreateMutex();
    if (led_mode_mutex == NULL) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: LedStrip led_mode_mutex could not be created!");
    }

    led_data_mutex = xSemaphoreCreateMutex();
    if (led_data_mutex == NULL) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: LedStrip led_data_mutex could not be created!");
    }

    // Start the frame timer
    frame_timer->initiate();
    DBG_PRINTLN(LedStrip, "<- LedStrip::begin()");
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
    if (brightness) {
        if (state_val) {
            brightness->turn_on();
        } else {
            brightness->turn_off();
        }
    }
    DBG_PRINTLN(LedStrip, "<- LedStrip::set_state()");
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
    DBG_PRINTF(LedStrip, "-> LedStrip::fill_all(color_rgb: {%u, %u, %u})\n", color_rgb[0], color_rgb[1], color_rgb[2]);
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
    DBG_PRINTLN(LedStrip, "<- LedStrip::fill_all()");
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
