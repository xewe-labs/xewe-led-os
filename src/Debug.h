// Debug.h
#pragma once

#include <Arduino.h>

// —————————————————————————————————————————————
// Per-class debug flags (0 = off, 1 = on)
// —————————————————————————————————————————————

// Hardware
#define DEBUG_Button            0
#define DEBUG_LedSignal         0
#define DEBUG_LedStrip          0
#define DEBUG_Transducer        0

// Resources
#define DEBUG_Memory            0
#define DEBUG_Storage           0

// Software
#define DEBUG_HomeKit           0
#define DEBUG_Alexa             0
#define DEBUG_YandexAlisa       0
#define DEBUG_WebServer         0

// LedController & friends
#define DEBUG_AsyncTimer        0
#define DEBUG_AsyncTimerArray   0
#define DEBUG_Brightness        0
#define DEBUG_PerlinFade        0
#define DEBUG_ColorSolid        0
#define DEBUG_Rainbow           0
#define DEBUG_ColorChanging     0
#define DEBUG_LedMode           0
#define DEBUG_LedController     1

// SystemController
#define DEBUG_CommandParser     0
#define DEBUG_SystemController  0

// Interfaces
#define DEBUG_Wifi              0
#define DEBUG_SerialPort        0

// —————————————————————————————————————————————
// Generic debug macros
// —————————————————————————————————————————————

// Helper to turn a token into DEBUG_<token>
#define DBG_ENABLED(cls)      (DEBUG_##cls)

// Print a plain line
#define DBG_PRINTLN(cls, msg)                     \
    do { if (DBG_ENABLED(cls)) Serial.println(msg); } while(0)

// Print a formatted string
#define DBG_PRINTF(cls, fmt, ...)                                             \
    do { if (DBG_ENABLED(cls)) Serial.printf((fmt), ##__VA_ARGS__); } while(0)
