# XeWe Led OS

---

#### The ultimate LED Strip Software for ESP32

---

# The problem

I have built many LED applications. In-between them I had a lot of repetitive work that I decided to distill in one piece of software — **XeWe LED OS**.

Instead of rewriting glue code for LEDs, WiFi, voice assistants, storage, and buttons every time, this project provides a reusable, modular operating system focused entirely on addressable LED strips.

---

# Features

* CLI commands via Serial Port to control addressable LED strip
* WiFi connectivity that allows:

  * Local Web Server for control via the web browser on the same network
  * Apple HomeKit + Siri control (requires hub: Apple TV or HomePod)
  * Alexa voice + app control (requires Alexa-enabled speaker)
* Physical buttons support
* Persistent settings via NVS (survive reboot and power loss)
* Modular architecture: enable or disable features at runtime

---

# Supported Hardware

* ESP32-C3
* ESP32-C6
* ESP32-S3

![IMG\_2737.webp](static/media/resources/readme/IMG_2737.webp)

---

# About the Features

## CLI Interface

The CLI is the core control mechanism of XeWe LED OS.

It provides **deterministic, scriptable, and debuggable** control over the system.
All other interfaces (Web, HomeKit, Alexa, Buttons) ultimately translate user input into CLI-style state updates.

**Command format**

```
$<module> <command> [param0] [param1] ... [paramN]
```

**Examples**

```
$led set_brightness 128
$led set_rgb 255 0 0
$wifi scan
```

**Rules**

* Parameters are space-separated
* Most numeric parameters are in range `0–255`
* Commands are case-sensitive
* Multiple commands can be sent sequentially

**Helpful commands**

* `$help` — list all available modules and commands
* `$system help`
* `$wifi help`
* `$led help`

---

## Web Interface UI

The Web Interface provides a **real-time browser-based control panel** available to any device on the same WiFi network.

**Capabilities**

* Hue slider (HSV-based)
* Brightness slider
* On / Off control
* Mode selection
* Live synchronization across all connected clients
* Automatic reconnect and heartbeat monitoring

**Technical details**

* HTTP server for REST-style commands
* WebSocket server for real-time updates
* Stateless clients: full state is pushed on connect
* No cloud dependency (LAN-only)

Access it at:

```
http://<device-ip>
```

---

## Apple HomeKit Support

XeWe LED OS integrates with **Apple Home** using HomeSpan.

**What you get**

* Native Home app control
* Siri voice commands
* Brightness and color control
* Appears as a standard color light accessory

**Requirements**

* Apple Home Hub (HomePod or Apple TV)
* iPhone / iPad for initial pairing

**Notes**

* Pairing is done once during setup
* Resetting HomeKit requires manual removal from the Home app
* If no hub is present, HomeKit control will stop working remotely

---

## Alexa

Alexa integration is provided via a modified Espalexa library.

**What you get**

* Voice control via Alexa
* Control via Alexa mobile app
* Brightness, color, and power state support

**Notes**

* Discovery-based setup (“Alexa, discover devices”)
* Requires an Alexa-enabled speaker on the same network
* Reset requires manual removal from the Alexa app

---

## Buttons

The Buttons module allows binding **physical GPIO buttons** to any CLI command.

**Features**

* Software debouncing
* Pull-up / pull-down configuration
* Multiple trigger modes:

  * `on_press`
  * `on_release`
  * `on_change`
* Each button can execute **any command**, including system commands

**Example**

```
$buttons add 9 "$led toggle_state" pullup on_press 50
```

This makes the system usable **without WiFi or voice assistants**, ideal for embedded or standalone installations.

---

## Quickstart

### Option 1: The Easy Way (Web Flasher)

Upload a precompiled binary file directly from your browser:

1. Go to **[maxdokukin.com/projects/xewe-led-os](https://maxdokukin.com/projects/xewe-led-os)**
2. Scroll down to **Firmware Flasher**.
3. Connect your board and follow the instructions.

### Option 2: Build from Source

To build and upload the code manually using the provided scripts:

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


---

## Software Modularity

All major features are implemented as **modules**.

Modules can be enabled or disabled at runtime:

```
$wifi disable
$homekit enable
```

**Common commands supported by all modules**

* `$<module> status`
* `$<module> reset`

**Optional commands**

* `$<module> enable`
* `$<module> disable`

This allows the system to scale from:

* fully standalone
* to LAN-only
* to voice-controlled smart home device

---

## Adding Your Code

If you want to extend XeWe LED OS:

* [CONTRIBUTING.md](doc/CONTRIBUTING.md)
* [ADDING_A_MODULE_OR_INTERFACE.md](doc/ADDING_A_MODULE_OR_INTERFACE.md)
* [MODULES_AND_INTERFACES.md](doc/MODULES_AND_INTERFACES.md)
* [PROJECT_STRUCTURE.md](doc/PROJECT_STRUCTURE.md)

The architecture is intentionally explicit and conservative to keep behavior predictable on embedded hardware.
