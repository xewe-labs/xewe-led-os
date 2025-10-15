/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



//#ifndef PERLIN_FADE_H
//#define PERLIN_FADE_H
//
//#include "../../../config/config.h"
//
//#include <FastLED.h>
//#include "../LedMode.h"
//#include "../../LedController/LedController.h"
//
//class PerlinFade : public LedMode {
//  private:
//    uint32_t counter = 0;
//
//    // Perlin Fade parameters
//    uint16_t hue_gap, adjusted_hue_gap, adjusted_hue_gap_half, half_hue_gap;
//    uint8_t fire_step, min_bright, max_bright, min_sat, max_sat;
//
//    AsyncTimer<uint16_t>* timer;       // Timer to manage frame updates
//
//    // Private methods
//    long get_fire_color(uint8_t val);
//
//  public:
//    // Constructor
//    PerlinFade(LedController* controller);
//
//    // Public methods
//    void frame() override;          // Override the virtual frame method from LedMode
//    bool is_done() override;        // Override the virtual is_done method from LedMode
//    uint8_t get_mode_id() override; // Override the virtual get_mode_id method from LedMode
//};
//
//#endif // PERLIN_FADE_H
