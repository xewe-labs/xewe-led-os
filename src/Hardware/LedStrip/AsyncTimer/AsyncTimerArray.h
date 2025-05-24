#ifndef ASYNCTIMERARRAY_H
#define ASYNCTIMERARRAY_H

#include <cstdint>
#include <Arduino.h>
#include <array>

class AsyncTimerArray {
private:
    uint32_t start_time, end_time, delay;
    uint8_t start_val[3], end_val[3];
    bool done = false, initiated = false;
    double progress = 0.0;

    void calculate_progress() {
        progress = (millis() - start_time) / static_cast<double>(delay);
        if (progress > 1.0) {
            done = true;
            progress = 1.0;
        }
    }

public:
    AsyncTimerArray(uint32_t delay)
        : delay(delay), start_time(millis()), end_time(start_time + delay) {
        memset(start_val, 0, 3);
        memset(end_val, 0, 3);
    }

    AsyncTimerArray(uint32_t delay, const uint8_t start_val[3], const uint8_t end_val[3])
        : delay(delay), start_time(millis()), end_time(start_time + delay) {
        memcpy(this->start_val, start_val, 3);
        memcpy(this->end_val, end_val, 3);
    }

    float get_progress() {
        calculate_progress();
        return progress;
    }

    std::array<uint8_t, 3> get_current_value() {
        calculate_progress();
        std::array<uint8_t, 3> current_val;
        for (int i = 0; i < 3; ++i) {
            current_val[i] = static_cast<uint8_t>(start_val[i] + (end_val[i] - start_val[i]) * progress);
//            Serial.print(i); Serial.print("   "); Serial.print(current_val[i]); Serial.print(" | ");
        }
//        Serial.println();
        return current_val;
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

    void reset(uint32_t new_delay, const uint8_t new_start_val[3], const uint8_t new_end_val[3]) {
        delay = new_delay;
        memcpy(start_val, new_start_val, 3);
        memcpy(end_val, new_end_val, 3);
        reset();
    }
};

#endif  // ASYNCTIMERARRAY_H
