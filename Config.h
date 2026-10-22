// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/Config.h
/*
 * Uncomment #define line and change the value to override the default.
 */
#pragma once

/************************/
/* ---- start core ---- */
/************************/
/* ---- start default runtime values ---- */

/* Device name (system/device_name), string literal, up to 254 characters; asked at first boot */
// #define XEWE_CORE_DEVICE_NAME "xewe-device"

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* Serial baud rate (SerialPortConfig::baud_rate), bits per second */
// #define XEWE_CORE_SERIAL_BAUD 115200

/* Serial TX buffer (SerialPortConfig::tx_buffer_size), bytes, set before Serial.begin */
// #define XEWE_CORE_SERIAL_TX_BUF 2048

/* Serial RX buffer (SerialPortConfig::rx_buffer_size), bytes; one input burst must fit */
// #define XEWE_CORE_SERIAL_RX_BUF 1024

/* Delay after opening the port (SerialPortConfig::startup_delay_ms), ms; 0 on a real UART */
// #define XEWE_CORE_SERIAL_DELAY_MS 1000

/* Echo typed characters back (SerialPortConfig::echo), 0 or 1 */
// #define XEWE_CORE_SERIAL_ECHO 1

/* Serial line buffer (SerialPort::INPUT_BUFFER_SIZE), bytes including the terminator, 2 to 256 */
// #define XEWE_CORE_INPUT_BUF_SIZE 255

/* Completed input lines queued (SerialPort::INPUT_QUEUE_LINES), 1 to 255 */
// #define XEWE_CORE_INPUT_QUEUE 4

/* AsyncTimer progress recompute interval, ms; 0 recomputes on every call */
// #define XEWE_CORE_TIMER_TICK_MS 5

/* ---- end compile time defined constants ---- */

/**********************/
/* ---- end core ---- */
/**********************/

/***********************/
/* ---- start led ---- */
/***********************/
/* ---- start default runtime values ---- */

/* LED has a clock line (led/use_clk), 0 or 1; asked at first boot */
// #define XEWE_LED_USE_CLK 0

/* LED chipset id (led/chip), 0 to 45 (README chipset table); asked at first boot */
// #define XEWE_LED_CHIP 0

/* Strip voltage (led/voltage), V, 5, 12 or 24; asked at first boot */
// #define XEWE_LED_VOLTAGE 5

/* Parallel strips (led/lines), 0 = one strip, to NUM_LEDS_MAX; asked at first boot */
// #define XEWE_LED_LINES 0

/* Number of LEDs, per line when lines > 0 (led/num_led), 0 to NUM_LEDS_MAX; asked at first boot */
// #define XEWE_LED_NUM_LED 600

/* Colour order (led/colorder), 0 to 5 = RGB RBG GRB GBR BRG BGR; asked at first boot */
// #define XEWE_LED_COLORDER 0

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* GPIO of the LED data line (every chipset); release images set it per board, see the README */
// #define XEWE_LED_PIN_DATA 4

/* GPIO of the LED clock line (used by the 14 clocked chipsets only) */
// #define XEWE_LED_PIN_CLOCK 3

/* Largest strip the firmware drives, LEDs; sizes the buffers, max fps about 1e6 / (value * 30) */
// #define XEWE_LED_NUM_LEDS_MAX 600

/* Cross-fade between two modes, ms */
// #define XEWE_LED_MODE_FADE_MS 900

/* Brightness and on/off fade, ms */
// #define XEWE_LED_BRIGHTNESS_FADE_MS 500

/* Frame period of the render task, ms; 20 ms = 50 fps */
// #define XEWE_LED_FRAME_MS 20

/* Window of the FPS figure in `$led status`, s */
// #define XEWE_LED_FPS_WINDOW_S 3

/* Capacity of the listener set (colour, brightness, state, mode and length events) */
#define XEWE_LED_LISTENERS 8

/* Stack of the render task, bytes */
// #define XEWE_LED_TASK_STACK 4096

/* FreeRTOS priority of the render task (loopTask is 1) */
// #define XEWE_LED_TASK_PRIORITY 3

/* CPU core of the render task (0 on single-core chips) */
// #define XEWE_LED_TASK_CORE 0

/* ---- end compile time defined constants ---- */

/*********************/
/* ---- end led ---- */
/*********************/

/************************/
/* ---- start wifi ---- */
/************************/
/* ---- start default runtime values ---- */

/* Network name (wifi/ssid), string literal, up to 32 characters; asked at first boot */
// #define XEWE_WIFI_SSID ""

/* Network password (wifi/psw), string literal, up to 63 characters; asked at first boot */
// #define XEWE_WIFI_PSW ""

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* ---- end compile time defined constants ---- */

/**********************/
/* ---- end wifi ---- */
/**********************/

/*********************************/
/* ---- start web-interface ---- */
/*********************************/
/* ---- start default runtime values ---- */

/* HTTP port (web_interface/port), 1 to 65535; takes effect after a restart */
// #define XEWE_WEB_INTERFACE_PORT 80

/* Path GET / redirects to (web_interface/root), string, up to 31 characters, "" = console */
// #define XEWE_WEB_INTERFACE_ROOT ""

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* ---- end compile time defined constants ---- */

/*******************************/
/* ---- end web-interface ---- */
/*******************************/

/***************************/
/* ---- start led-web ---- */
/***************************/
/* ---- start default runtime values ---- */

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* ---- end compile time defined constants ---- */

/*************************/
/* ---- end led-web ---- */
/*************************/

/************************/
/* ---- start time ---- */
/************************/
/* ---- start default runtime values ---- */

/* Timezone offset (time/tz_gmt_str), string literal GMT+HH:MM; detected, asked at first boot */
// #define XEWE_TIME_TZ_GMT_STR "GMT+00:00"

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* ---- end compile time defined constants ---- */

/**********************/
/* ---- end time ---- */
/**********************/
