// File: ColorSolid.h
#ifndef COLORSOLID_H
#define COLORSOLID_H

#include "../LedMode.h"
#include "../../LedStrip.h"

class ColorSolid : public LedMode {
public:
    ColorSolid(LedStrip* led_strip, uint8_t r, uint8_t g, uint8_t b);
    ~ColorSolid() override = default;

    void    frame()         override;
    bool    is_done()       override;
    uint8_t get_mode_id()   override;
    String  get_mode_name() override;

    std::array<uint8_t, 3> get_target_rgb() override;
    uint8_t  get_target_r()   override;
    uint8_t  get_target_g()   override;
    uint8_t  get_target_b()   override;
private:
    id_done_flag = false;
};

#endif  // COLORSOLID_H
