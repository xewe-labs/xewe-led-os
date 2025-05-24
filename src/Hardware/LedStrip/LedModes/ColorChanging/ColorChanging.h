#ifndef COLORCHANGING_H
#define COLORCHANGING_H

#include "../LedMode.h"
#include "../../LedStrip.h"

class ColorChanging : public LedMode {
private:
    AsyncTimerArray* timer;

public:
    ColorChanging(LedStrip* controller, uint8_t current_r, uint8_t current_g, uint8_t current_b, uint8_t target_t0, uint8_t target_t1, uint8_t target_t2, char mode, uint32_t time);

    void frame() override;
    bool is_done() override;
    uint8_t get_mode_id() override;
};

#endif  // ColorChanging_H
