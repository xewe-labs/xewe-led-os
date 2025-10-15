/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



#ifndef ASYNCTIMER_H
#define ASYNCTIMER_H

#include "../../../../Debug.h"

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
        // This is a frequently called helper, extensive logging is not advised.
        // DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::calculate_progress()");
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
        // DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::calculate_progress()");
    }

public:
    AsyncTimer(uint32_t delay, T start = T(), T target = T())
        : delay_ms(delay), start_val(start), target_val(target) {
        DBG_PRINTF(AsyncTimer, "-> AsyncTimer::AsyncTimer(delay: %lu, start: %f, target: %f)\n", delay, static_cast<double>(start), static_cast<double>(target));
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::AsyncTimer()");
    }
    ~AsyncTimer() = default;

    void initiate() {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::initiate()");
        start_time = millis();
        if (start_time > calc_interval_ms)
            last_calc_time = start_time - calc_interval_ms;
        else
            last_calc_time = 0;
        progress = 0.0;
        done = false;
        initiated = true;
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::initiate()");
    }

    T get_start_value() const {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::get_start_value()");
        DBG_PRINTF(AsyncTimer, "<- AsyncTimer::get_start_value() returns: %f\n", static_cast<double>(start_val));
        return start_val;
    }

    T get_current_value() const {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::get_current_value()");
        calculate_progress();
        T result = done ? target_val : T(start_val + (target_val - start_val) * progress);
        DBG_PRINTF(AsyncTimer, "<- AsyncTimer::get_current_value() returns: %f\n", static_cast<double>(result));
        return result;
    }

    T get_target_value() const {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::get_target_value()");
        DBG_PRINTF(AsyncTimer, "<- AsyncTimer::get_target_value() returns: %f\n", static_cast<double>(target_val));
        return target_val;
    }

    bool is_done() const {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::is_done()");
        calculate_progress();
        DBG_PRINTF(AsyncTimer, "<- AsyncTimer::is_done() returns: %s\n", done ? "true" : "false");
        return done;
    }

    bool is_active() const {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::is_active()");
        calculate_progress();
        bool result = initiated && !done;
        DBG_PRINTF(AsyncTimer, "<- AsyncTimer::is_active() returns: %s\n", result ? "true" : "false");
        return result;
    }

    void terminate() {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::terminate()");
        initiated = false;
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::terminate()");
    }

    void reset() {
        DBG_PRINTLN(AsyncTimer, "-> AsyncTimer::reset()");
        done = false;
        progress = 0.0;
        initiated = false;
        last_calc_time = 0;
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::reset()");
    }

    void reset(T new_start, T new_target) {
        DBG_PRINTF(AsyncTimer, "-> AsyncTimer::reset(new_start: %f, new_target: %f)\n", static_cast<double>(new_start), static_cast<double>(new_target));
        start_val = new_start;
        target_val = new_target;
        reset();
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::reset(start, target)");
    }

    void reset(uint32_t new_delay, T new_start, T new_target) {
        DBG_PRINTF(AsyncTimer, "-> AsyncTimer::reset(new_delay: %lu, new_start: %f, new_target: %f)\n", new_delay, static_cast<double>(new_start), static_cast<double>(new_target));
        delay_ms = new_delay;
        start_val = new_start;
        target_val = new_target;
        reset();
        DBG_PRINTLN(AsyncTimer, "<- AsyncTimer::reset(delay, start, target)");
    }
};

#endif  // ASYNCTIMER_H