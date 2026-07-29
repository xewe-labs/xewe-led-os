// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
 // Debug.h

#pragma once


#define DEBUG_ModuleController  0
#define DEBUG_Module            0

#define DEBUG_SerialPort        0
#define DEBUG_Nvs               0
#define DEBUG_System            0
#define DEBUG_CommandExecutor   0

#define DEBUG_Wifi              0
#define DEBUG_WebInterface      0

#define DEBUG_Time              0
#define DEBUG_Scheduler         0


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
