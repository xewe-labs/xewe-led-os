#include "LedStrip.h"
// Assuming LedMode.h, ColorSolid.h, ColorChanging.h are included via LedStrip.h or directly as needed.
// Assuming Debug.h is available.

LedStrip::LedStrip()
    : leds(nullptr), // Initialize pointer to nullptr
      num_led(0),
      led_mode_mutex(NULL),
      led_data_mutex(NULL) {
    DBG_PRINTLN(LedStrip, "LedStrip: Constructor called (empty)");
}

// The begin() method now accepts the leds_ptr and handles all initialization.
void LedStrip::begin(CRGB* leds_ptr, uint16_t count) {
    DBG_PRINTLN(LedStrip, "LedStrip: begin() called");

    // Assign the hardware pointer
    this->num_led = count;   // <--- FIX: Set the initial length
    this->leds = leds_ptr;
    if (this->leds == nullptr) {
        DBG_PRINTLN(LedStrip, "FATAL ERROR: leds_ptr provided to begin() is null!");
        // Handle this critical error, as the object is unusable without a valid LED array.
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
}

LedStrip::~LedStrip() {
    if (led_mode_mutex != NULL) {
        vSemaphoreDelete(led_mode_mutex);
        led_mode_mutex = NULL;
    }
    if (led_data_mutex != NULL) {
        vSemaphoreDelete(led_data_mutex);
        led_data_mutex = NULL;
    }
    DBG_PRINTLN(LedStrip, "LedStrip: Destructor called, mutexes deleted");
}

void LedStrip::loop() {
    if (frame_timer->is_active()) {
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
}

void LedStrip::set_mode(uint8_t new_mode_id) {
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
}

void LedStrip::set_rgb(std::array<uint8_t, 3> new_rgb) {
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
}

void LedStrip::set_r(uint8_t r_val) { // Renamed parameter to avoid conflict with member if any
    // This method calls public getters and setters which are (or will be) thread-safe.
    // No additional mutex needed here IF get_g/get_b/set_rgb are fully thread-safe.
    // However, to prevent race condition of getting G and B, then another thread changing them
    // before set_rgb is called with the combined value, it's safer to lock.
    if (xSemaphoreTake(led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        uint8_t current_g = 0;
        uint8_t current_b = 0;
        if (led_mode) {
            current_g = led_mode->get_g();
            current_b = led_mode->get_b();
        }
        // Store values before releasing or calling another function that might lock
        xSemaphoreGive(led_mode_mutex);
        set_rgb({r_val, current_g, current_b}); // set_rgb will take the lock again
    } else {
         DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in set_r");
    }
}

void LedStrip::set_g(uint8_t g_val) {
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
}

void LedStrip::set_b(uint8_t b_val) {
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
}

void                    LedStrip::set_hsv                         (std::array<uint8_t, 3> new_hsv) {
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
}

void LedStrip::set_h(uint8_t h_val) {
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
}

void LedStrip::set_s(uint8_t s_val) {
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
}

void LedStrip::set_v(uint8_t v_val) {
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
}

void LedStrip::set_brightness(uint8_t new_brightness) {
    // Calls to brightness object, which is assumed to be internally thread-safe
    if (brightness) {
        brightness->set_brightness(new_brightness);
    }
}

void LedStrip::set_state(uint8_t state_val) { // Renamed parameter
    // Calls to brightness object, which is assumed to be internally thread-safe
    if (brightness) {
        if (state_val) {
            brightness->turn_on();
        } else {
            brightness->turn_off();
        }
    }
}

void LedStrip::turn_on() {
    // Calls to brightness object, which is assumed to be internally thread-safe
    if (brightness) {
        brightness->turn_on();
    }
}

void LedStrip::turn_off() {
    // Calls to brightness object, which is assumed to be internally thread-safe
    if (brightness) {
        brightness->turn_off();
    }
}

void                    LedStrip::fill_all                        (std::array<uint8_t, 3> color_rgb) {
    if (xSemaphoreTake(led_data_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint16_t i = 0; i < num_led; i++) {
            // set_all_strips_pixel_color is an internal helper, called while lock is held
            set_all_strips_pixel_color(i, color_rgb);
        }
        if (num_led > 0) { // Only show if there are LEDs
            FastLED.show();
        }
        xSemaphoreGive(led_data_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_data_mutex in fill_all");
    }
}

// This is an internal helper, called by fill_all which holds led_data_mutex.
// It accesses 'leds' (safe) and calls 'brightness' methods (thread-safe).
void LedStrip::set_all_strips_pixel_color (uint16_t i, std::array<uint8_t, 3> color_rgb) {
    if (leds && i < num_led) { // num_led is read by caller under lock
         // Calls to brightness object methods are assumed to be thread-safe internally
        if (brightness) {
            leds[i] = CRGB(brightness->get_dimmed_color(color_rgb[0]),
                           brightness->get_dimmed_color(color_rgb[1]),
                           brightness->get_dimmed_color(color_rgb[2]));
        } else {
            leds[i] = CRGB(color_rgb[0], color_rgb[1], color_rgb[2]); // Fallback if brightness object is null
        }
    }
}

void LedStrip::set_length(uint16_t new_length) { // Renamed parameter
    if (xSemaphoreTake(led_data_mutex, portMAX_DELAY) == pdTRUE) {
        // Clear existing LEDs directly before changing num_led
        if (leds) {
            for (uint16_t i = 0; i < num_led; i++) { // Use current num_led
                leds[i] = CRGB::Black;
            }
            if (num_led > 0) {
                FastLED.show(); // Show cleared strip
            }
        }
        num_led = new_length; // Update num_led under lock
        DBG_PRINTF(LedStrip, "Set num_led to %u\n", num_led);
        xSemaphoreGive(led_data_mutex);
    } else {
        DBG_PRINTLN(LedStrip, "ERROR: Could not take led_data_mutex in set_length");
    }
}

uint16_t                LedStrip::get_length                      () const {
    return num_led;
}

// --- Getters for led_mode properties ---
// Using const_cast as LedStrip mutexes are not declared mutable.

std::array<uint8_t,3> LedStrip::get_rgb() const {
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_rgb();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_rgb"); }
    return res;
}

uint8_t LedStrip::get_r() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_r();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_r"); }
    return res;
}

uint8_t LedStrip::get_g() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_g();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_g"); }
    return res;
}

uint8_t LedStrip::get_b() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_b();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_b"); }
    return res;
}

std::array<uint8_t,3> LedStrip::get_target_rgb() const {
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_rgb();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_rgb"); }
    return res;
}

uint8_t LedStrip::get_target_r() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_r();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_r"); }
    return res;
}

uint8_t LedStrip::get_target_g() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_g();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_g"); }
    return res;
}

uint8_t LedStrip::get_target_b() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_b();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_b"); }
    return res;
}

std::array<uint8_t, 3> LedStrip::get_hsv() const {
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_hsv();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_hsv"); }
    return res;
}

uint8_t LedStrip::get_h() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_h();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_h"); }
    return res;
}

uint8_t LedStrip::get_s() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_s();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_s"); }
    return res;
}

uint8_t LedStrip::get_v() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_v();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_v"); }
    return res;
}

std::array<uint8_t, 3> LedStrip::get_target_hsv() const {
    std::array<uint8_t,3> res = {0,0,0};
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_hsv();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_hsv"); }
    return res;
}

uint8_t LedStrip::get_target_h() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_h();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_h"); }
    return res;
}

uint8_t LedStrip::get_target_s() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_s();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_s"); }
    return res;
}

uint8_t LedStrip::get_target_v() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_target_v();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_target_v"); }
    return res;
}

uint8_t LedStrip::get_brightness() const {
    // Assumes brightness object is internally thread-safe
    if (brightness) {
        return brightness->get_last_brightness();
    }
    return 0; // Default if brightness object is null
}

bool LedStrip::get_state() const {
    // Assumes brightness object is internally thread-safe
    if (brightness) {
        return brightness->get_state();
    }
    return false; // Default if brightness object is null
}

String                  LedStrip::get_mode_name                   () const {
    String res = "";
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_mode_name();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_mode_name"); }
    return res;
}


uint8_t LedStrip::get_mode_id() const {
    uint8_t res = 0;
    if (xSemaphoreTake(const_cast<LedStrip*>(this)->led_mode_mutex, portMAX_DELAY) == pdTRUE) {
        if (led_mode) res = led_mode->get_mode_id();
        xSemaphoreGive(const_cast<LedStrip*>(this)->led_mode_mutex);
    } else { DBG_PRINTLN(LedStrip, "ERROR: Could not take led_mode_mutex in get_mode_id"); }
    return res;
}


//todo
//needs work!
uint8_t                 LedStrip::get_target_brightness           () const {
    return brightness->get_target_value();
}
bool                    LedStrip::get_target_state                () const {
    brightness->get_state();
}
uint8_t                 LedStrip::get_target_mode_id              () const {
    get_mode_id();
}
String                  LedStrip::get_target_mode_name            () const {
    get_mode_name();
}
