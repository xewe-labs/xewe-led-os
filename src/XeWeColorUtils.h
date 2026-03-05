/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
#pragma once

#include <array>
#include <FastLED.h>

namespace xewe::color {

/**
 * Converts HSV to RGB using FastLED's rainbow spectrum.
 * Input: std::array{H, S, V}
 */
inline std::array<uint8_t, 3> hsv_to_rgb(const std::array<uint8_t, 3>& hsv) {
    CRGB rgb;
    hsv2rgb_rainbow(CHSV(hsv[0], hsv[1], hsv[2]), rgb);
    return {rgb.r, rgb.g, rgb.b};
}

/**
 * Converts RGB to HSV using FastLED's approximate conversion.
 * Input: std::array{R, G, B}
 */
inline std::array<uint8_t, 3> rgb_to_hsv(const std::array<uint8_t, 3>& rgb) {
    CHSV hsv = rgb2hsv_approximate(CRGB(rgb[0], rgb[1], rgb[2]));
    return {hsv.h, hsv.s, hsv.v};
}

} // namespace xewe::color