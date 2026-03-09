/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// Debug.h
#pragma once

#include <Arduino.h>

// core
#define DEBUG_Module            0
#define DEBUG_SerialPort        0
#define DEBUG_System            0
#define DEBUG_CommandParser     0
#define DEBUG_Wifi              0
#define DEBUG_Buttons           0

// Led
#define DEBUG_AsyncTimer        0
#define DEBUG_Brightness        0
#define DEBUG_LedStrip          0

#define DEBUG_ModeController    0
#define DEBUG_Mode              0
#define DEBUG_Solid             0
#define DEBUG_Fade              0
#define DEBUG_Rainbow           0

// Interfaces
#define DEBUG_Interface         0
#define DEBUG_Nvs               0
#define DEBUG_WebInterface      0
#define DEBUG_HomeKit           0
#define DEBUG_Alexa             0


#define DBG_ENABLED(cls)      (DEBUG_##cls)

#define DBG_PRINTLN(cls, msg)                                    \
    do { if (DBG_ENABLED(cls)) {                                 \
            Serial.print("[DBG] [");                             \
            Serial.print(#cls); /* <--- Changed here */          \
            Serial.print("]: ");                                 \
            Serial.println(msg);                                 \
        } } while(0)

#define DBG_PRINTF(cls, fmt, ...)                                \
    do { if (DBG_ENABLED(cls)) {                                 \
            Serial.print("[DBG] [");                             \
            Serial.print(#cls); /* <--- Changed here */          \
            Serial.print("]: ");                                 \
            Serial.printf((fmt), ##__VA_ARGS__);                 \
        } } while(0)
