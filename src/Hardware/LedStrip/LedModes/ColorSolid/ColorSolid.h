// File: ColorSolid.h
#ifndef COLORSOLID_H
#define COLORSOLID_H

#include "../LedMode.h"
#include "../../LedStrip.h"

class ColorSolid : public LedMode {
public:
    ColorSolid(LedStrip* controller);
    ColorSolid(LedStrip* controller, uint8_t r, uint8_t g, uint8_t b);
    ~ColorSolid() override = default;

    void    frame()       override;
    bool    is_done()     override;
    uint8_t get_mode_id() override;

    // Return a pointer into LedMode’s shared rgb[]
    uint8_t* get_target_rgb() override;
    uint8_t  get_target_r()   override;
    uint8_t  get_target_g()   override;
    uint8_t  get_target_b()   override;
};

#endif  // COLORSOLID_H
