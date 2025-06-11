#include "Brightness.h"

Brightness::Brightness(uint16_t transition_delay, uint8_t initial_brightness, uint8_t state_param) // Renamed 'state' parameter to 'state_param' to avoid conflict with member 'state'
    : state(state_param), last_brightness(initial_brightness)
{
    DBG_PRINTF(Brightness, "Brightness::Brightness() - delay=%u, initial_brightness=%u, state=%u\n",
               transition_delay, initial_brightness, this->state); // Use this->state

    internal_mutex = xSemaphoreCreateMutex();
    if (internal_mutex == NULL) {
        // Handle error: mutex not created
        DBG_PRINTLN(Brightness, "FATAL ERROR: Brightness internal_mutex could not be created!");
        // Consider how to handle this failure, e.g., assert, log, prevent object use
    }

    // The following logic uses member variables (state, last_brightness)
    // and unique_ptr 'timer'. These operations are part of the object's
    // construction and are safe before the object is usable by other threads.
    // No explicit mutex lock needed here for these initial assignments.
    if (this->state) { // Use this->state
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, last_brightness, initial_brightness);
        DBG_PRINTF(Brightness, "  Created timer for ON state: start=%u, target=%u\n",
                   last_brightness, initial_brightness);
    } else {
        timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay, 0, 0);
        DBG_PRINTLN(Brightness, "  Created timer for OFF state: start=0, target=0");
    }

    timer->initiate();
    // Accessing timer methods after creation is fine within the constructor context
    DBG_PRINTF(Brightness, "  Timer initiated: get_start_value()=%u, get_target_value()=%u\n",
               timer->get_start_value(), timer->get_target_value());
}

 Brightness::~Brightness() {
     if (internal_mutex != NULL) {
         vSemaphoreDelete(internal_mutex);
     }
 }

uint8_t Brightness::get_start_value() const {
    uint8_t v = 0; // Default value in case mutex take fails
    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        v = timer->get_start_value();
        DBG_PRINTF(Brightness, "get_start_value() -> %u\n", v);
        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_start_value");
    }
    return v;
}

uint8_t Brightness::get_current_value() const {
    uint8_t v = 0; // Default value
    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        v = timer->get_current_value();
        //    DBG_PRINTF(Brightness, "get_current_value() -> %u (done=%s)\n",
        //               v, timer->is_done() ? "true" : "false");
        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_current_value");
    }
    return v;
}

uint8_t Brightness::get_target_value() const {
    uint8_t v = 0; // Default value
    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        v = timer->get_target_value();
        DBG_PRINTF(Brightness, "get_target_value() -> %u\n", v);
        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_target_value");
    }
    return v;
}

void Brightness::set_brightness(uint8_t new_brightness) {
    // WARNING: This method is called by turn_on() and turn_off().
    // If internal_mutex is not recursive, this will cause a deadlock.
    if (xSemaphoreTake(internal_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTF(Brightness, "set_brightness(%u) called (state=%s)\n",
                   new_brightness, state ? "ON" : "OFF");

        if (state) {
            uint8_t current = timer->get_current_value(); // Reading timer state
            DBG_PRINTF(Brightness, "  Resetting timer: current_value=%u, new_target=%u\n",
                       current, new_brightness);
            timer->reset(current, new_brightness); // Modifying timer state
            timer->initiate();                     // Modifying timer state
            DBG_PRINTLN(Brightness, "  Timer re-initiated after set_brightness");
        }

        if (new_brightness) { // Update last_brightness if setting a non-zero value, or if turning on
            last_brightness = new_brightness;
            DBG_PRINTF(Brightness, "  last_brightness updated -> %u\n", last_brightness);
        }
        xSemaphoreGive(internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in set_brightness");
    }
}

void Brightness::turn_on() {
    // WARNING: This method calls set_brightness().
    // If internal_mutex is not recursive, this will cause a deadlock.
    if (xSemaphoreTake(internal_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTLN(Brightness, "turn_on() called");
        if (state) {
            DBG_PRINTLN(Brightness, "  Already on");
            xSemaphoreGive(internal_mutex); // Release mutex before early return
            return;
        }
        DBG_PRINTLN(Brightness, "  Was off, turning on");
        state = true;
        uint8_t brightness_to_set = last_brightness; // Read last_brightness under lock
        // Release mutex before calling another public method that will take it
        xSemaphoreGive(internal_mutex);

        set_brightness(brightness_to_set); // This call will attempt to take the mutex again

        // Lock again if further state changes are needed, or ensure DBG prints are safe
        if (xSemaphoreTake(internal_mutex, portMAX_DELAY) == pdTRUE) {
            DBG_PRINTLN(Brightness, "  State set to ON after set_brightness call");
            xSemaphoreGive(internal_mutex);
        }

    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in turn_on");
    }
}

void Brightness::turn_off() {
    // WARNING: This method calls set_brightness().
    // If internal_mutex is not recursive, this will cause a deadlock.
    if (xSemaphoreTake(internal_mutex, portMAX_DELAY) == pdTRUE) {
        DBG_PRINTLN(Brightness, "turn_off() called");
        if (!state) {
            DBG_PRINTLN(Brightness, "  Already off");
            xSemaphoreGive(internal_mutex); // Release mutex before early return
            return;
        }
        DBG_PRINTLN(Brightness, "  Turning off (set brightness to 0)");
        // state will be set to false *after* calling set_brightness(0)
        // last_brightness is intentionally not changed to 0 by this specific call path
        // so it remembers the previous brightness when turned on again.

        // Release mutex before calling another public method that will take it
        xSemaphoreGive(internal_mutex);

        set_brightness(0); // This call will attempt to take the mutex again

        // Lock again to safely change state
        if (xSemaphoreTake(internal_mutex, portMAX_DELAY) == pdTRUE) {
            state = false;
            DBG_PRINTLN(Brightness, "  State set to OFF after set_brightness call");
            xSemaphoreGive(internal_mutex);
        }

    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in turn_off");
    }
}

uint8_t Brightness::get_dimmed_color(uint8_t color) const {
    // WARNING: This method calls get_current_value().
    // If internal_mutex is not recursive, this will cause a deadlock if get_current_value also locks.
    uint8_t result = 0; // Default value
    bool local_state;
    bool timer_is_done;
    uint8_t current_timer_val;

    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        local_state = this->state;
        // Access timer methods while holding the lock
        timer_is_done = timer->is_done();
        current_timer_val = timer->get_current_value(); // Get current value from timer while locked

        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_dimmed_color (part 1)");
        return 0; // Early exit if mutex cannot be taken
    }

    if (!local_state && timer_is_done) {
        // DBG_PRINTF(Brightness, "get_dimmed_color(%u) -> OFF state, returning 0\n", color);
        return 0;
    }

    // current_timer_val was already fetched under lock
    result = static_cast<uint8_t>((static_cast<uint32_t>(color) * current_timer_val) / 255); // Use uint32_t for intermediate multiplication

    // DBG_PRINTF(Brightness, "get_dimmed_color(%u): current_value=%u -> result=%u\n",
    //            color, current_timer_val, result);
    return result;
}

bool Brightness::get_state() const {
    bool s = false; // Default value
    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        s = state;
        DBG_PRINTF(Brightness, "get_state() -> %s\n", s ? "true" : "false");
        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_state");
    }
    return s;
}

uint8_t Brightness::get_last_brightness() const {
    uint8_t lb = 0; // Default value
    if (xSemaphoreTake(const_cast<Brightness*>(this)->internal_mutex, portMAX_DELAY) == pdTRUE) {
        lb = last_brightness;
        DBG_PRINTF(Brightness, "get_last_brightness() -> %u\n", lb);
        xSemaphoreGive(const_cast<Brightness*>(this)->internal_mutex);
    } else {
        DBG_PRINTLN(Brightness, "ERROR: Could not take internal_mutex in get_last_brightness");
    }
    return lb;
}