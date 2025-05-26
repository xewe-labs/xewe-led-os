#ifndef ASYNCTIMER_H
#define ASYNCTIMER_H

#include <cstdint>
#include <Arduino.h>
#include "../../../Debug.h"

template <typename T>
class AsyncTimer {
private:
    uint32_t start_time, end_time, delay;
    T start_val, end_val;
    bool done = false, initiated = false;
    double progress = 0.0;

public:
    AsyncTimer(uint32_t delay)
        : delay(delay), start_time(millis()), end_time(start_time + delay), start_val(T()), end_val(T()) {}

    AsyncTimer(uint32_t delay, T start_val, T end_val)
        : delay(delay), start_time(millis()), end_time(start_time + delay), start_val(start_val), end_val(end_val) {}

    void calculate_progress() {
        progress = (millis() - start_time) / static_cast<double>(delay);
        if (progress > 1.0) {
            done = true;
            progress = 1.0;
        }
    }

    double get_progress() {
        calculate_progress();
        return progress;
    }

    T get_current_value() {
        calculate_progress();
        return start_val + (end_val - start_val) * progress;
    }

    T get_target_value() const {
        DBG_PRINTLN(AsyncTimer, "T get_target_value() const {");
        return end_val;
    }
    T get_start_value() const { return start_val; }

    bool is_done() {
        calculate_progress();
        return done;
    }

    bool is_active() {
        calculate_progress();
        return !done;
    }

    bool is_initiated() const { return initiated; }
    bool is_not_initiated() const { return !initiated; }

    void initiate() { initiated = true; }
    void terminate() { initiated = false; }

    void reset() {
        start_time = millis();
        end_time = start_time + delay;
        done = false;
        initiated = false;
    }

    void reset(uint32_t new_delay) {
        delay = new_delay;
        reset();
    }

    void reset(T new_start_val, T new_end_val) {
        start_val = new_start_val;
        end_val = new_end_val;
        reset();
    }

    void reset(uint32_t new_delay, T new_start_val, T new_end_val) {
        delay = new_delay;
        start_val = new_start_val;
        end_val = new_end_val;
        reset();
    }

    void set_delay(uint32_t new_delay){
        delay = new_delay;
    }
};

#endif  // ASYNCTIMER_H
