/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Interfaces/InterfaceName/InterfaceName.h
#pragma once

#include "../../Interface/Interface.h"
#include "../../../Config.h"
#include "../../../Debug.h"


struct InterfaceNameConfig : public ModuleConfig {};


class InterfaceName : public Interface {
public:
    explicit                    InterfaceName              (SystemController& controller);

    // required implementation
    void                        sync_color                  (std::array<uint8_t,3> color)   override;
    void                        sync_brightness             (uint8_t brightness)            override;
    void                        sync_state                  (uint8_t state)                 override;
    void                        sync_mode                   (uint8_t mode)                  override;
    void                        sync_length                 (uint16_t length)               override;

    // optional implementation
//    void                        sync_all                    (std::array<uint8_t,3> color,
//                                                             uint8_t brightness,
//                                                             uint8_t state,
//                                                             uint8_t mode,
//                                                             uint16_t length)               override;
//    void                begin_routines_required     (const ModuleConfig& cfg)       override;
//    void                begin_routines_init         (const ModuleConfig& cfg)       override;
//    void                begin_routines_regular      (const ModuleConfig& cfg)       override;
//    void                begin_routines_common       (const ModuleConfig& cfg)       override;
//
//    void                loop                        ()                              override;
//
//    void                reset                       (const bool verbose=false)      override;
//
//    void                enable                      (const bool verbose=false)      override;
//    void                disable                     (const bool verbose=false)      override;
//
//    std::string         status                      (const bool verbose=false)      const override;
//    bool                is_enabled                  (const bool verbose=false)      const override;
//    bool                is_disabled                 (const bool verbose=false)      const override;
//    bool                init_setup_complete         (const bool verbose=false)      const override;

    // other methods

private:

};
