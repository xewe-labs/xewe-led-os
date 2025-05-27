#ifndef ASYNCTIMERARRAY_H
#define ASYNCTIMERARRAY_H

#include <cstdint>
#include <Arduino.h>
#include <array>
#include <cstring>

class AsyncTimerArray {
private:
    uint32_t start_time;
    uint32_t end_time;
    uint32_t delay;
    uint8_t  start_val[3];
    uint8_t  end_val[3];
    bool     done = false;
    bool     initiated = false;
    double   progress = 0.0;

    void calculate_progress() {
        progress = (millis() - start_time) / static_cast<double>(delay);
        if (progress > 1.0) {
            done = true;
            progress = 1.0;
        }
    }

public:
    // Construct with delay, start/end zeroed
    AsyncTimerArray(uint32_t delay_ms)
        : delay(delay_ms),
          start_time(millis()),
          end_time(start_time + delay_ms)
    {
        memset(start_val, 0, sizeof(start_val));
        memset(end_val,   0, sizeof(end_val));
    }

    // Construct with explicit start/end values
    AsyncTimerArray(uint32_t delay_ms,
                    const uint8_t start_vals[3],
                    const uint8_t end_vals[3])
        : delay(delay_ms),
          start_time(millis()),
          end_time(start_time + delay_ms)
    {
        memcpy(start_val, start_vals, sizeof(start_val));
        memcpy(end_val,   end_vals,   sizeof(end_val));
    }

    // Returns interpolation progress [0.0,1.0]
    float get_progress() {
        calculate_progress();
        return static_cast<float>(progress);
    }

    // Compute and return current interpolated values by value
    std::array<uint8_t, 3> get_current_value() {
        calculate_progress();
        std::array<uint8_t, 3> current{};
        for (int i = 0; i < 3; ++i) {
            current[i] = static_cast<uint8_t>(
                start_val[i] + (end_val[i] - start_val[i]) * progress
            );
        }
        return current;
    }

    // Return final target values by value
    std::array<uint8_t, 3> get_target_value() const {
        return { end_val[0], end_val[1], end_val[2] };
    }

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

    void initiate()  { initiated = true; }
    void terminate() { initiated = false; }

    void reset() {
        start_time = millis();
        end_time   = start_time + delay;
        done       = false;
        initiated  = false;
    }

    void reset(uint32_t new_delay_ms) {
        delay = new_delay_ms;
        reset();
    }

    void reset(uint32_t new_delay_ms,
               const uint8_t new_start[3],
               const uint8_t new_end[3]) {
        delay = new_delay_ms;
        memcpy(start_val, new_start, sizeof(start_val));
        memcpy(end_val,   new_end,   sizeof(end_val));
        reset();
    }
};

#endif  // ASYNCTIMERARRAY_H
