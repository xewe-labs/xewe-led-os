// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Software/SmartHome/Alexa/Alexa.h
#pragma once

#include <Espalexa.h>
#include <WebServer.h>
#include <array>
#include <string>

#include "../../../Module/SyncModule.h"


struct AlexaConfig : public ModuleConfig {};

class Alexa : public SyncModule {
public:
    explicit        Alexa                   (ModuleController& controller);

    // required implementation
    void            sync_color              (std::array<uint8_t, 3> color)   override;
    void            sync_brightness         (uint8_t brightness)             override;
    void            sync_state              (bool state)                     override;
    void            sync_mode               (uint8_t mode)                   override;
    void            sync_length             (uint16_t length)                override;

    // optional implementation
    void            sync_all                (std::array<uint8_t, 3> color,
                                             uint8_t                brightness,
                                             bool                   state,
                                             uint8_t                mode,
                                             uint16_t               length)  override;
    void            begin_routines_required (const ModuleConfig& cfg)        override;
    void            begin_routines_init     (const ModuleConfig& cfg)        override;
    void            loop                    ()                               override;
    void            reset                   (const bool verbose      = false,
                                             const bool do_restart   = true,
                                             const bool keep_enabled = true) override;

    Espalexa&       get_instance            ();

private:
    void            update_event            (EspalexaDevice* device_ptr);

    Espalexa        espalexa;
    EspalexaDevice* device                  = nullptr;
};
