# XeWe LED OS
<img src="static/media/resources/main_img.png" alt="main_img.png" height="400">

XeWe LED OS is modular firmware for addressable LED strips on ESP32 boards. It provides reusable control, connectivity, and persistence features so LED projects do not need custom glue code for WiFi, voice assistants, storage, and buttons.

## Overview

The project is an operating system for LED strip applications built on ESP32-C3, ESP32-C6, and ESP32-S3. It centers on a serial CLI and layers additional interfaces on top of it, including a local web UI, Apple HomeKit, Alexa, physical buttons, and persistent settings stored in NVS.

## Features

* Serial CLI for direct LED and system control
* Local WiFi web interface for browser-based control
* Apple HomeKit and Siri integration
* Alexa voice and app control
* Physical GPIO button bindings to CLI commands
* Persistent settings stored in NVS
* Runtime-enableable modular architecture

## Installation

### Prerequisites

* Supported hardware:

  * ESP32-C3
  * ESP32-C6
  * ESP32-S3
* For web flashing:

  * A compatible browser
  * A connected ESP32 board
* For source builds:

  * Git
  * Shell environment for the provided build scripts or Arduino IDE
* For Apple HomeKit:

  * Apple TV or HomePod as a home hub
  * Apple Device for pairing
* For Alexa:

  * An Alexa-enabled speaker on the same network

### Setup

1. Choose one installation method.

2. To flash a precompiled binary from the browser, open:

   * `https://maxdokukin.com/projects/xewe-led-os`
   * Scroll to **Firmware Flasher**
   * Connect the board and follow the instructions

3. To build from source with the provided scripts:

```bash
# clone
git clone https://githib.com/maxdokukin/xewe-led-os
cd xewe-led-os/build/scripts

# set up build environment
./setup_build_enviroment_mac.sh
# OR ./setup_build_enviroment_linux.sh

# print the port esp is connected to
ls /dev/cu.*

# build: ./build.sh -c <target_chip> -p <port>
./build.sh -c c3                                # build
./build.sh -c c3 -p /dev/cu.usbmodem11143201    # build and upload
```

4. To build from source with Arduino IDE:

   1. Set up Arduino IDE for ESP32 development.
   2. Clone the repository:

      * `git clone https://githib.com/maxdokukin/xewe-led-os`
   3. Open the project in the IDE.
   4. Install these libraries:

      * `https://github.com/FastLED/FastLED.git --branch 3.10.3`
      * `https://github.com/maxdokukin/xewe-led-library-espalexa`
      * `https://github.com/maxdokukin/xewe-led-library-homespan`
      * `https://github.com/maxdokukin/xewe-led-library-websockets`
      * `https://github.com/bblanchon/ArduinoJson`
   5. Move the libraries to `Arduino/libraries`.
   6. Compile and upload.

## Usage

The CLI is the primary control interface. Commands use this format:

```text
$<module> <command> [param0] [param1] ... [paramN]
```

Common examples:

```text
$led set_brightness 128
$led set_rgb 255 0 0
$wifi scan
```

Helpful commands:

```text
$help
$system help
$wifi help
$led help
```

Command rules:

* Parameters are space-separated
* Most numeric parameters are in the range `0-255`
* Commands are case-sensitive
* Multiple commands can be sent sequentially

To use the local web interface after connecting the device to WiFi, open:

```text
http://<device-ip>
```

Example button binding:

```text
$buttons add 9 "$led toggle_state" pullup on_press 50
```

## Configuration

* Modules can be enabled or disabled at runtime:

```text
$wifi disable
$homekit enable
```

* Common module commands:

  * `$<module> status`
  * `$<module> reset`

* Some modules also support:

  * `$<module> enable`
  * `$<module> disable`

* Button triggers:

  * `on_press`
  * `on_release`
  * `on_change`

* Web interface behavior:

  * LAN-only access
  * Stateless clients receive full state on connect
  * WebSocket-based live synchronization and reconnect handling

## Project Structure

* `build/scripts` - shell scripts for building and uploading firmware
* `doc/CONTRIBUTING.md` - contribution guidance
* `doc/ADDING_A_MODULE_OR_INTERFACE.md` - module and interface extension guide
* `doc/MODULES_AND_INTERFACES.md` - module architecture reference
* `doc/PROJECT_STRUCTURE.md` - repository structure documentation

## API

The web interface exposes local network control through:

* HTTP server for REST-style commands
* WebSocket server for real-time state updates

The device state is synchronized across connected clients, and full state is pushed when a client connects.

## Commands

* `$help` - list available modules and commands
* `$system help` - show system command help
* `$wifi help` - show WiFi command help
* `$led help` - show LED command help
* `$led set_brightness <value>` - set brightness
* `$led set_rgb <r> <g> <b>` - set RGB color
* `$wifi scan` - scan for WiFi networks
* `$<module> status` - show module status
* `$<module> reset` - reset a module
* `$<module> enable` - enable a module when supported
* `$<module> disable` - disable a module when supported

## Contributing

See the project documentation for extension and contribution guidance:

* `doc/CONTRIBUTING.md`
* `doc/ADDING_A_MODULE_OR_INTERFACE.md`
* `doc/MODULES_AND_INTERFACES.md`
* `doc/PROJECT_STRUCTURE.md`
