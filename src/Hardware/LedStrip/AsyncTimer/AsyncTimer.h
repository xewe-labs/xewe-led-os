#ifndef ASYNCTIMER_H
#define ASYNCTIMER_H

#include "../../../Debug.h"

template<typename T>
class AsyncTimer {
    static_assert(std::is_arithmetic_v<T>,
                  "AsyncTimer<T> requires an arithmetic type");

private:
    mutable uint32_t start_time{};
    mutable uint32_t last_calc_time{};
    static constexpr uint32_t calc_interval_ms = 5;

    uint32_t delay_ms;
    T start_val, target_val;

    mutable bool done{false}, initiated{false};
    mutable double progress{0.0};

    void calculate_progress() const {
        if (done || !initiated) return;

        uint32_t now = millis();
        // Throttle: skip if we ran less than calc_interval_ms ago
        if (now - last_calc_time < calc_interval_ms) return;
        last_calc_time = now;

        if (start_val == target_val) {
            done = true;
            progress = 1.0;
            return;
        }

        uint32_t elapsed = now - start_time;
        progress = double(elapsed) / delay_ms;
        if (progress >= 1.0) {
            done = true;
            progress = 1.0;
        }
    }

public:
    AsyncTimer(uint32_t delay, T start = T(), T target = T())
        : delay_ms(delay), start_val(start), target_val(target) {}
    ~AsyncTimer() = default;

    void initiate() {
        start_time = millis();
        if (start_time > calc_interval_ms)
            last_calc_time = start_time - calc_interval_ms;
        else
            last_calc_time = 0;
        progress = 0.0;
        done = false;
        initiated = true;
    }

    T get_start_value() const {
        return start_val;
    }

    T get_current_value() const {
        calculate_progress();
        return done ? target_val : T(start_val + (target_val - start_val) * progress);
    }

    T get_target_value() const {
        return target_val;
    }

    bool is_done() const {
        calculate_progress();
        return done;
    }

    bool is_active() const {
        calculate_progress();
        return initiated && !done;
    }

    void terminate() {
        initiated = false;
    }

    void reset() {
        done = false;
        progress = 0.0;
        initiated = false;
        last_calc_time = 0;
    }

    void reset(T new_start, T new_target) {
        start_val = new_start;
        target_val = new_target;
        reset();
    }

    void reset(uint32_t new_delay, T new_start, T new_target) {
        delay_ms = new_delay;
        start_val = new_start;
        target_val = new_target;
        reset();
    }
};

#endif  // ASYNCTIMER_H
