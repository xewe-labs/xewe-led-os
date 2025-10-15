/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



//#include "PerlinFade.h"
//
//// Constructor
//PerlinFade::PerlinFade(LedController* controller)
//    : LedMode(controller), counter(0) { // Initialize base class and members

//    led_controller = controller;
//
//    // Initialize PerlinFade parameters using configuration values
//    timer->reset(LedMode::get_hue() * 255, LedMode::get_hue() * 255, get_config_value("perlin_hue_transition_delay"));
//    hue_gap = get_config_value("perlin_hue_gap");
//    fire_step = get_config_value("perlin_fire_step");
//    min_bright = get_config_value("perlin_min_bright");
//    max_bright = get_config_value("perlin_max_bright");
//    min_sat = get_config_value("perlin_min_sat");
//    max_sat = get_config_value("perlin_max_sat");
//    half_hue_gap = hue_gap / 2;
//
//    // Adjust hue gap
//    adjusted_hue_gap = hue_gap;
//    adjusted_hue_gap_half = adjusted_hue_gap / 2;
//}
//
//// Frame update method
//void PerlinFade::frame() {
//    for (int i = 0; i < led_controller->num_led; i++) {
//        uint32_t color = get_fire_color(inoise8(i * fire_step, counter));
//
//        // Extract red, green, and blue components
//        uint8_t r = (color >> 16) & 0xFF;
//        uint8_t g = (color >> 8) & 0xFF;
//        uint8_t b = color & 0xFF;
//
//        // Set the color on the LED controller
//        led_controller->set_pixel(i, r, g, b);
//    }
//    counter += 5;
//}
//
//// Check if the effect is done (override the base class method)
//bool PerlinFade::is_done() {
//    // Assuming the effect is never done; implement your condition if needed
//    return false;
//}
//
//// Return the mode ID for this effect (override the base class method)
//uint8_t PerlinFade::get_mode_id() {
//    // Assign a unique ID for this mode
//    return 3; // Example ID, change if needed
//}
//
//// Get fire color based on value
//long PerlinFade::get_fire_color(uint8_t val) {
//    return led_controller->led_strip->ColorHSV(
//        timer->get_current_value() - adjusted_hue_gap_half + map(val, 0, 255, 0, adjusted_hue_gap),
//        constrain(map(val, 0, 255, max_sat, min_sat), 0, 255),
//        constrain(map(val, 0, 255, min_bright, max_bright), 0, 255)
//    );
//}
