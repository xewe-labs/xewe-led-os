/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/ModeController.h

#pragma once

#include "../../../../../Config.h"
#include "../../../Core/Nvs/Nvs.h"
#include "../../../../Utils/XeWeTimer.h"
#include "ModeRegistry/ModeRegistry.h"
#include "Modes/Mode/Mode.h"

#include <algorithm>
#include <limits>
#include <utility>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Nvs;

class ModeController {
public:
    ModeController                                          (CRGB* output_buffer,
                                                             uint16_t num_leds,
                                                             uint16_t transition_delay_ms,
                                                             Nvs& nvs,
                                                             std::string_view nvs_namespace = "led_strip");

    void                        loop                        ();

    // Mode Management
    void                        set_mode                    (const uint8_t mode_id, const std::map<std::string, uint16_t>& params = {});
    bool                        set_mode_param              (std::string_view key, uint16_t value);
    void                        set_rgb                     (const std::array<uint8_t, 3> new_rgb);
    void                        set_hsv                     (const std::array<uint8_t, 3> new_hsv);

    bool                        adj_mode_param              (std::string_view key, int32_t value_delta);

    void                        reset_current_mode          ();

    // Getters
    std::array<uint8_t, 3>      get_rgb                     () const { return current_mode->get_rgb(); }
    uint8_t                     get_current_mode_id         () const { return current_mode->get_id(); }
    std::string_view            get_current_mode_name       () const { return current_mode->get_name(); }
    std::vector<ModeParam>      get_current_mode_params     () const { return current_mode->get_params(); }
    const ModeConfig&           get_current_mode_config     () const { return current_mode->get_config(); }
    uint16_t                    get_current_mode_param      (std::string_view key) const;

    const ModeConfig&           get_mode_config             (uint8_t mode_id) const;
    std::string                 get_all_modes_json          () const;

    uint16_t                    get_mode_transition_delay   () const { return transition_timer->get_delay_ms(); }

    // Setters
    void                        set_length                  (const uint16_t new_num_leds) { num_leds = new_num_leds; }

private:
    void                        update_interpolate_buffers  (CRGB* output_buffer_ref);
    std::map<std::string, uint16_t> get_params_as_map       () const;

    std::map<std::string, uint16_t> get_default_params_for_mode(uint8_t mode_id) const;
    std::map<std::string, uint16_t> load_mode_params_from_nvs(uint8_t mode_id) const;
    void                        persist_mode_params_to_nvs  (uint8_t mode_id) const;
    std::string                 make_nvs_param_key          (uint8_t mode_id, std::string_view param_key) const;
    uint16_t                    normalize_mode_param_value  (std::string_view key, int32_t value) const;

    uint16_t                    num_leds;
    std::unique_ptr<AsyncTimer<uint8_t>> transition_timer;

    std::unique_ptr<Mode>       current_mode;
    std::unique_ptr<Mode>       old_mode;

    CRGB*                       output_buffer;

    std::array<CRGB, LED_STRIP_NUM_LEDS_MAX> buffer_current;
    std::array<CRGB, LED_STRIP_NUM_LEDS_MAX> buffer_old;

    bool                        buffer_old_static_flag;

    Nvs&                        nvs;
    std::string                 nvs_namespace;
};