// File: ColorChanging.h
#ifndef COLORCHANGING_H
#define COLORCHANGING_H

#include "../LedMode.h"
#include "../../LedStrip.h"
#include "../../AsyncTimer/AsyncTimerArray.h"

class ColorChanging : public LedMode {
public:
    ColorChanging                           (LedStrip* led_strip,
                                             uint8_t current_r, uint8_t current_g, uint8_t current_b,
                                             uint8_t t0, uint8_t t1, uint8_t t2,
                                             char mode, uint32_t duration_ms);
    ~ColorChanging                          () override = default;

    void                    loop            () override;
    bool                    is_done         () override;
    uint8_t                 get_mode_id     () override;
    String                  get_mode_name   () override;
    uint8_t                 get_target_mode_id     () override;
    String                  get_target_mode_name   () override;

    std::array<uint8_t, 3>  get_target_rgb  () override;
    uint8_t                 get_target_r    () override;
    uint8_t                 get_target_g    () override;
    uint8_t                 get_target_b    () override;

    std::array<uint8_t, 3>  get_target_hsv  () override;
    uint8_t                 get_target_h    () override;
    uint8_t                 get_target_s    () override;
    uint8_t                 get_target_v    () override;

private:
    std::unique_ptr<AsyncTimerArray>            timer;
};

#endif  // COLORCHANGING_H
