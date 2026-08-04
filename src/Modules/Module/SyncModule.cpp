// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/SyncModule.cpp

#include "SyncModule.h"


void SyncModule::sync_all(std::array<uint8_t, 3> color,
                          uint8_t brightness,
                          uint8_t state,
                          uint8_t mode,
                          uint16_t length) {
    DBG_PRINTLN(SyncModule, "sync_all(): Syncing all parameters.");
    if (is_disabled(false)) return;
    sync_state(state);
    sync_length(length);
    sync_mode(mode);
    sync_brightness(brightness);
    sync_color(color);
    DBG_PRINTLN(SyncModule, "sync_all(): Sync complete.");
}

void SyncModule::sync_param(std::string key,
                            uint8_t value) {}