// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/Config.h
//
// Build settings and the modules' compile-time values. `xewe build` generates <XeWeBuildInfo.h>
// (PROJECT_NAME, BUILD_VERSION, BUILD_TIMESTAMP, BUILD_CHIP, XEWE_CHIP_<C3|C6|S3> and every
// `--define KEY=VALUE`); it wins over the defaults below. The version lives in xewe.toml [project];
// do not edit it here.
#pragma once

// Unconditional on purpose: arduino-cli only adds a library to the build when an #include of it
// fails, so a __has_include() guard would silently drop the generated header. Plain Arduino IDE
// (no xewe): delete this line; the defaults below then apply.
#include <XeWeBuildInfo.h>

#ifndef PROJECT_NAME
#define PROJECT_NAME "xewe-led-os"
#endif

#ifndef BUILD_VERSION
#define BUILD_VERSION "0.0.0"
#endif

#ifndef BUILD_TIMESTAMP
#define BUILD_TIMESTAMP __DATE__ " " __TIME__
#endif

#ifndef BUILD_CHIP
#define BUILD_CHIP "unknown"
#endif

// Printed in the boot header. Override: xewe build --define 'PROJECT_URL="https://..."'
#ifndef PROJECT_URL
#define PROJECT_URL "https://github.com/xewe-labs/xewe-led-os"
#endif

// Serial console baud rate (the tools' `xewe serial` defaults to 115200 as well).
#ifndef SERIAL_BAUD_RATE
#define SERIAL_BAUD_RATE 115200
#endif

// Product values for the led module. They come before the generated module blocks below, so they
// win over those blocks' defaults; `xewe build --define` wins over both.
// Listener slots of the led module: led-web, HomeKit, Alexa and Home Assistant use one each; the
// rest is headroom.
#ifndef XEWE_MODULE_LED_LISTENERS
#define XEWE_MODULE_LED_LISTENERS 8
#endif

// Largest strip the firmware drives (LEDs).
#ifndef XEWE_MODULE_LED_NUM_LEDS_MAX
#define XEWE_MODULE_LED_NUM_LEDS_MAX 600
#endif

// Module values. `./setup.sh` and `xewe modules select` append the compile-time values of every
// selected module below, one marked block per module (`// ---- <slug> (xewe modules generate) ----`
// to `// ---- end <slug> ----`). Edit the numbers; keep the markers. A block is added once and
// never rewritten. `xewe build` passes this file to every translation unit, so the module sources
// see your values.

// ---- led (xewe modules generate) ----
// Values of the led module. Edit the numbers; keep this marker and the end marker.
// GPIO of the LED data line (every chipset).
#ifndef XEWE_MODULE_LED_PIN_DATA
#if defined(XEWE_CHIP_C3)
#define XEWE_MODULE_LED_PIN_DATA 4
#elif defined(XEWE_CHIP_C6)
#define XEWE_MODULE_LED_PIN_DATA 20
#elif defined(XEWE_CHIP_S3)
#define XEWE_MODULE_LED_PIN_DATA 13
#else
#define XEWE_MODULE_LED_PIN_DATA 48
#endif
#endif

// GPIO of the LED clock line (used by the 14 clocked chipsets only).
#ifndef XEWE_MODULE_LED_PIN_CLOCK
#if defined(XEWE_CHIP_C3)
#define XEWE_MODULE_LED_PIN_CLOCK 3
#elif defined(XEWE_CHIP_C6)
#define XEWE_MODULE_LED_PIN_CLOCK 19
#elif defined(XEWE_CHIP_S3)
#define XEWE_MODULE_LED_PIN_CLOCK 12
#else
#define XEWE_MODULE_LED_PIN_CLOCK 12
#endif
#endif

// Largest strip the firmware drives (LEDs). Sizes the pixel and cross-fade buffers; every frame
// sends this many pixels, so max fps is about 1e6 / (value * 30) for one-wire chips.
#ifndef XEWE_MODULE_LED_NUM_LEDS_MAX
#define XEWE_MODULE_LED_NUM_LEDS_MAX 600
#endif

// Cross-fade between two modes (ms).
#ifndef XEWE_MODULE_LED_MODE_FADE_MS
#define XEWE_MODULE_LED_MODE_FADE_MS 900
#endif

// Brightness and on/off fade (ms).
#ifndef XEWE_MODULE_LED_BRIGHTNESS_FADE_MS
#define XEWE_MODULE_LED_BRIGHTNESS_FADE_MS 500
#endif

// Frame period of the render task (ms); 20 ms = 50 fps.
#ifndef XEWE_MODULE_LED_FRAME_MS
#define XEWE_MODULE_LED_FRAME_MS 20
#endif

// Window of the FPS figure in `$led status` (s).
#ifndef XEWE_MODULE_LED_FPS_WINDOW_S
#define XEWE_MODULE_LED_FPS_WINDOW_S 3
#endif

// Capacity of the listener set (other modules notified of colour, brightness, state, mode, length).
#ifndef XEWE_MODULE_LED_LISTENERS
#define XEWE_MODULE_LED_LISTENERS 4
#endif

// Stack of the render task (bytes).
#ifndef XEWE_MODULE_LED_TASK_STACK
#define XEWE_MODULE_LED_TASK_STACK 4096
#endif

// FreeRTOS priority of the render task (loopTask is 1).
#ifndef XEWE_MODULE_LED_TASK_PRIORITY
#define XEWE_MODULE_LED_TASK_PRIORITY 3
#endif

// CPU core of the render task (0 on single-core chips).
#ifndef XEWE_MODULE_LED_TASK_CORE
#define XEWE_MODULE_LED_TASK_CORE 0
#endif
// ---- end led ----

// ---- wifi (xewe modules generate) ----
// Values of the wifi module. Edit the numbers; keep this marker and the end marker.
// Debug output of the module (0 or 1). 1 prints every step, credentials included: bench only.
#ifndef XEWE_MODULE_WIFI_DEBUG
#define XEWE_MODULE_WIFI_DEBUG 0
#endif
// ---- end wifi ----

// ---- led-web (xewe modules generate) ----
// Values of the led-web module. Edit the numbers; keep this marker and the end marker.
// TCP port of the WebSocket server that pushes LED changes to the page. The page connects to
// ws://<host>:81/ (the port is written in its script), so change both together.
#ifndef XEWE_MODULE_LED_WEB_WS_PORT
#define XEWE_MODULE_LED_WEB_WS_PORT 81
#endif

// Interval of the "H" heartbeat frame while a page is connected (ms). The page shows Offline and
// reloads itself when no heartbeat arrives for 2200 ms.
#ifndef XEWE_MODULE_LED_WEB_HEARTBEAT_MS
#define XEWE_MODULE_LED_WEB_HEARTBEAT_MS 1000
#endif
// ---- end led-web ----
