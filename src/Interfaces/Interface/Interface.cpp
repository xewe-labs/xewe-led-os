/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/XeWe-LED-OS
 *********************************************************************************/



// src/Interfaces/Interface/Interface.cpp

#include "Interface.h"

void Interface::sync_all(std::array<uint8_t,3> color,
                   uint8_t brightness,
                   uint8_t state,
                   uint8_t mode,
                   uint16_t length)
{
    DBG_PRINTLN(Nvs, "sync_all(): Syncing all parameters to NVS.");
    if (is_disabled(false)) return;
    sync_state(state);
    sync_length(length);
    sync_mode(mode);
    sync_brightness(brightness);
    sync_color(color);
    DBG_PRINTLN(Nvs, "sync_all(): Sync complete.");
}