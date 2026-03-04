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

#include "Modes/Mode/Mode.h"
#include "Modes/Solid/Solid.h"
#include "Modes/Rainbow/Rainbow.h"

struct ModeConfig {
    uint8t mode_id;
    string mode_name;
    std vector ModeParam params;
}

struct ModeParam {
    key,
    display name,
    min_value,
    max_value,
    default_value,
    step_value,
}

class ModeController : public Module {
public:
    ModeController                                          (CRGB* output_buffer, uint16_t num_leds, uint16_t transition_delay_ms);

    void                        begin_routines_required     (const ModuleConfig& cfg)       override;
    void                        begin_routines_init         (const ModuleConfig& cfg)       override;
    void                        begin_routines_regular      (const ModuleConfig& cfg)       override;
    void                        begin_routines_common       (const ModuleConfig& cfg)       override;

    void                        loop                        ();
    void                        set_mode                    (const uint8_t mode);
    void                        set_mode_param              (stringview key, uint16_t value);
    void                        set_color                   (const array<uint8_t, 3> new_rgb);

    ModeConfig                  get_current_mode_config     () const;

    void                        disable                     (const bool verbose=false,
                                                             const bool do_restart=true)    override;
    void                        reset                       (const bool verbose=false,
                                                             const bool do_restart=true,
                                                             const bool keep_enabled=true)    override;

    string                      status                      (const bool verbose=false)      const override;

    // custom functions template
    void                        custom_function             ();

private:
    uint16_t                    num_leds;
    uint16_t                    transition_delay_ms;

    // State
    bool is_transitioning;
    bool using_snapshot;
    uint32_t transition_start_time;

    // Smart pointers to manage mode lifecycles elegantly
    std::unique_ptr<Mode> current_mode;
    std::unique_ptr<Mode> old_mode;

    CRGB* output_buffer;
    std::vector<CRGB> buffer_current;
    std::vector<CRGB> buffer_old;
};
