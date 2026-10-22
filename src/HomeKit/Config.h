// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// xewe-led-os/src/HomeKit/Config.h
/*
The values of HomeKit (id homekit, prefix XEWE_HOMEKIT_), in the module form of XeWeCore's
doc/config.md. A --define or a line in the project's Config.h wins. The QR image is
doc/media/HomeKit_Connect_QR.png in this repository, served from main; it encodes HomeSpan's
default setup code 466-37-726 and setup ID HSPN.
*/
#pragma once

/* ---- start default runtime values ---- */

/* ---- end default runtime values ---- */

/* ---- start compile time defined constants ---- */

/* Link to the setup QR image printed at first boot (the serial console cannot show an image) */
#ifndef XEWE_HOMEKIT_QR_URL
#define XEWE_HOMEKIT_QR_URL \
    "https://github.com/xewe-labs/xewe-led-os/raw/main/" \
    "doc/media/HomeKit_Connect_QR.png"
#endif

/* ---- end compile time defined constants ---- */
