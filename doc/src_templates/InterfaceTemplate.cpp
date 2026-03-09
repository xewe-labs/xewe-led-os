/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/<hardware|software>/InterfaceName/InterfaceName.cpp

#include "InterfaceName.h"
#include "../../../SystemController/SystemController.h"

InterfaceName::InterfaceName(SystemController& controller)
      : Interface(controller,
               /* interface_name        */  "",
               /* interface_description */  "",
               /* nvs_key               */  "",
               /* requires_init_setup   */  false,
               /* can_be_disabled       */  false,
               /* has_cli_cmds          */  false)
{}

void InterfaceName::sync_color (array<uint8_t,3> color) {
    if (is_disabled()) return;

    // do your custom routines here
}

void InterfaceName::sync_brightness (uint8_t brightness) {
    if (is_disabled()) return;

    // do your custom routines here
}

void InterfaceName::sync_state (uint8_t state) {
    if (is_disabled()) return;

    // do your custom routines here
}

void InterfaceName::sync_mode (uint8_t mode) {
    if (is_disabled()) return;

    // do your custom routines here
}

void InterfaceName::sync_length (uint16_t length) {
    if (is_disabled()) return;

    // do your custom routines here
}

void InterfaceName::custom_function () {
    // make sure to have this check, otherwise if other modules call it when disabled, this will lead to undesired bugs.
    if (is_disabled()) return;

    // do your custom routines here
}
