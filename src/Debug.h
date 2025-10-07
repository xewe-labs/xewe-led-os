// Debug.h
#pragma once

#include <Arduino.h>

// —————————————————————————————————————————————
// Per-class debug flags (0 = off, 1 = on)
// —————————————————————————————————————————————

// Hardware
#define DEBUG_Button            0
#define DEBUG_LedSignal         0

// Resources
#define DEBUG_Nvs               1

// Software
#define DEBUG_Homekit           0
#define DEBUG_Alexa             0
#define DEBUG_YandexAlisa       0

// LedController & friends
#define DEBUG_AsyncTimer        0
#define DEBUG_AsyncTimerArray   0
#define DEBUG_Brightness        0
#define DEBUG_PerlinFade        0
#define DEBUG_ColorSolid        0
#define DEBUG_Rainbow           0
#define DEBUG_ColorChanging     0
#define DEBUG_LedMode           0
#define DEBUG_LedStrip          1

// SystemController
#define DEBUG_CommandParser     1
#define DEBUG_SystemController  1
#define DEBUG_System            1

// Interfaces
#define DEBUG_Wifi              0
#define DEBUG_SerialPort        1
#define DEBUG_Web               1


#define DEBUG_Module            0
#define DEBUG_Interface         0



// —————————————————————————————————————————————
// Generic debug macros
// —————————————————————————————————————————————

// Helper to turn a token into DEBUG_<token>
#define DBG_ENABLED(cls)      (DEBUG_##cls)

// Print a plain line with “[DBG]: ” prefix
// Print a plain line with “[DBG] [ClassName]: ” prefix
#define DBG_PRINTLN(cls, msg)                                    \
    do { if (DBG_ENABLED(cls)) {                                 \
            Serial.print("[DBG] [");                             \
            Serial.print(#cls); /* <--- Changed here */          \
            Serial.print("]: ");                                 \
            Serial.println(msg);                                 \
        } } while(0)

// Print a formatted string with “[DBG] [ClassName]: ” prefix
#define DBG_PRINTF(cls, fmt, ...)                                \
    do { if (DBG_ENABLED(cls)) {                                 \
            Serial.print("[DBG] [");                             \
            Serial.print(#cls); /* <--- Changed here */          \
            Serial.print("]: ");                                 \
            Serial.printf((fmt), ##__VA_ARGS__);                 \
        } } while(0)

