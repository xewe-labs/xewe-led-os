#ifndef ASYNCTIMERARRAY_H
#define ASYNCTIMERARRAY_H

#include <array>
#include "../../../../Debug.h"

class AsyncTimerArray {
    static_assert(std::is_same_v<uint8_t, std::array<uint8_t,3>::value_type>,
                  "AsyncTimerArray works on 3-byte arrays of uint8_t");

private:
    mutable uint32_t start_time{};
    mutable uint32_t last_calc_time{};
    static constexpr uint32_t calc_interval_ms = 5;

    uint32_t delay_ms;
    std::array<uint8_t,3> start_val{}, target_val{};
    mutable bool done{false}, initiated{false};
    mutable double progress{0.0};

    void calculate_progress() const {
        // This is a frequently called helper, so logging is minimal.
        // DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::calculate_progress()");
        if (done || !initiated) return;

        uint32_t now = millis();
        if (now - last_calc_time < calc_interval_ms) return;
        last_calc_time = now;

        if (start_val == target_val) {
            done     = true;
            progress = 1.0;
            // DBG_PRINTLN(AsyncTimerArray, "   Progress calculated: done (start == target)");
            return;
        }

        uint32_t elapsed = now - start_time;
        progress = double(elapsed) / delay_ms;
        if (progress >= 1.0) {
            done     = true;
            progress = 1.0;
            // DBG_PRINTLN(AsyncTimerArray, "   Progress calculated: done (elapsed >= delay)");
        }
        // DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::calculate_progress()");
    }

public:
    /**
     * @param delay   Transition duration in milliseconds.
     * @param start   Starting RGB values (default all 0).
     * @param target  Target RGB values (default all 0).
     */
    AsyncTimerArray(uint32_t delay,
                    const std::array<uint8_t,3>& start  = {},
                    const std::array<uint8_t,3>& target = {})
        : delay_ms(delay), start_val(start), target_val(target)
    {
        DBG_PRINTF(AsyncTimerArray, "-> AsyncTimerArray::AsyncTimerArray(delay: %lu, start: {%u,%u,%u}, target: {%u,%u,%u})\n",
                   delay, start[0], start[1], start[2], target[0], target[1], target[2]);
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::AsyncTimerArray()");
    }
    ~AsyncTimerArray() = default;

    /** Begin (or restart) the transition. */
    void initiate() {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::initiate()");
        start_time     = millis();
        if (start_time > calc_interval_ms)
            last_calc_time = start_time - calc_interval_ms;
        else
            last_calc_time = 0;
        progress       = 0.0;
        done           = false;
        initiated      = true;
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::initiate()");
    }

    std::array<uint8_t,3> get_start_value() const {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::get_start_value()");
        DBG_PRINTF(AsyncTimerArray, "<- AsyncTimerArray::get_start_value() returns: {%u,%u,%u}\n", start_val[0], start_val[1], start_val[2]);
        return start_val;
    }

    /** Current interpolated RGB triple. */
    std::array<uint8_t,3> get_current_value() const {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::get_current_value()");
        calculate_progress();
        std::array<uint8_t,3> cur{};
        for (int i = 0; i < 3; ++i) {
            cur[i] = static_cast<uint8_t>(
                start_val[i] + (target_val[i] - start_val[i]) * progress
            );
        }
        DBG_PRINTF(AsyncTimerArray, "<- AsyncTimerArray::get_current_value() returns: {%u,%u,%u}\n", cur[0], cur[1], cur[2]);
        return cur;
    }

    /** Final target RGB triple. */
    std::array<uint8_t,3> get_target_value() const {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::get_target_value()");
        DBG_PRINTF(AsyncTimerArray, "<- AsyncTimerArray::get_target_value() returns: {%u,%u,%u}\n", target_val[0], target_val[1], target_val[2]);
        return target_val;
    }

    /** True once interpolation has reached the end. */
    bool is_done() const {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::is_done()");
        calculate_progress();
        DBG_PRINTF(AsyncTimerArray, "<- AsyncTimerArray::is_done() returns: %s\n", done ? "true" : "false");
        return done;
    }

    /** True while a transition is in progress. */
    bool is_active() const {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::is_active()");
        calculate_progress();
        bool result = !done && initiated;
        DBG_PRINTF(AsyncTimerArray, "<- AsyncTimerArray::is_active() returns: %s\n", result ? "true" : "false");
        return result;
    }

    /** Stop without completing (keeps current state). */
    void terminate() {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::terminate()");
        initiated = false;
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::terminate()");
    }

    /** Reset to un-initiated state (will recalc on next initiate). */
    void reset() {
        DBG_PRINTLN(AsyncTimerArray, "-> AsyncTimerArray::reset()");
        done            = false;
        progress        = 0.0;
        initiated       = false;
        last_calc_time  = 0;
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::reset()");
    }

    /** Reset with new start/target but same delay. */
    void reset(const std::array<uint8_t,3>& new_start,
               const std::array<uint8_t,3>& new_target)
    {
        DBG_PRINTF(AsyncTimerArray, "-> AsyncTimerArray::reset(new_start: {%u,%u,%u}, new_target: {%u,%u,%u})\n",
                   new_start[0], new_start[1], new_start[2], new_target[0], new_target[1], new_target[2]);
        start_val  = new_start;
        target_val = new_target;
        reset();
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::reset(start, target)");
    }

    /** Reset with entirely new delay, start, and target. */
    void reset(uint32_t new_delay,
               const std::array<uint8_t,3>& new_start,
               const std::array<uint8_t,3>& new_target)
    {
        DBG_PRINTF(AsyncTimerArray, "-> AsyncTimerArray::reset(new_delay: %lu, new_start: {%u,%u,%u}, new_target: {%u,%u,%u})\n",
                   new_delay, new_start[0], new_start[1], new_start[2], new_target[0], new_target[1], new_target[2]);
        delay_ms   = new_delay;
        start_val  = new_start;
        target_val = new_target;
        reset();
        DBG_PRINTLN(AsyncTimerArray, "<- AsyncTimerArray::reset(delay, start, target)");
    }
};

#endif  // ASYNCTIMERARRAY_H