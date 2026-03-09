/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/

// NOTE that the system MAX FPS depends on LED_STRIP_NUM_LEDS_MAX
// and as calculated as max_fps = 1000000 / (LED_STRIP_NUM_LEDS_MAX * 30)
// going over 1000 leds will drop fps too low for the effects to look good

#define PIN_LED_STRIP               9
#define LED_STRIP_TYPE              WS2815
#define LED_STRIP_COLOR_ORDER       RGB
#define LED_STRIP_NUM_LEDS_MAX      600
