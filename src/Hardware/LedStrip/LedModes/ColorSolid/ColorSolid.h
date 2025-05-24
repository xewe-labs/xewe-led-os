#ifndef COLORSOLID_H
#define COLORSOLID_H

#include "../LedMode.h"
#include "../../LedStrip.h"

class ColorSolid : public LedMode {
public:
    ColorSolid(LedStrip* controller);
    ColorSolid(LedStrip* controller, uint8_t r, uint8_t g, uint8_t b);

    void frame() override;
    bool is_done() override;
    uint8_t get_mode_id() override;
};

#endif  // COLORSOLID_H
