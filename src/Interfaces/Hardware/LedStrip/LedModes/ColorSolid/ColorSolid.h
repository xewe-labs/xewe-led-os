/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// File: ColorSolid.h
#ifndef COLORSOLID_H
#define COLORSOLID_H

#include "../LedMode.h"
#include "../../LedStrip.h"

class ColorSolid : public LedMode {
public:
    ColorSolid                              (LedStrip* led_strip, uint8_t r, uint8_t g, uint8_t b);
    ~ColorSolid                             () override = default;

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
    bool                    id_done_flag = false;
};

#endif  // COLORSOLID_H
