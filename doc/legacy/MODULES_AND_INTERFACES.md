# Module & Command Reference

The XeWe OS functions through independent modules. Below is a detailed breakdown of each module and its available commands, listed in initialization order.

## SerialPort Module

**Internal Infrastructure**

This module handles UART communication. It manages non-blocking I/O, input buffering, and formatting for the CLI. It serves as the primary interface for user interaction and debugging output.

---

## System Module

**Prefix:** `$system`

The System module acts as the kernel of the OS. It manages the lifecycle of other modules, handles system-wide resets, and provides hardware identification information (MAC, UID, Stack usage).

| Command       | Description                                                            | Sample Usage      |
| :------------ | :--------------------------------------------------------------------- | :---------------- |
| **`status`**  | Get the current status of the System module.                           | `$system status`  |
| **`reset`**   | Reset the System module logic.                                         | `$system reset`   |
| **`restart`** | Soft restart the ESP32.                                                | `$system restart` |
| **`reboot`**  | Alias for restart.                                                     | `$system reboot`  |
| **`info`**    | Displays Chip model, revision, and Build info.                         | `$system info`    |
| **`mac`**     | Prints the device MAC addresses.                                       | `$system mac`     |
| **`uid`**     | Generates a unique Device UID from the eFuse base MAC (and SHA256-64). | `$system uid`     |
| **`stack`**   | Prints the current task stack watermark (in words).                    | `$system stack`   |

---

## CommandParser Module

**Internal Infrastructure**

This is the text processing engine of the OS. It takes raw string input from the Serial Port or Web Interface, tokenizes the arguments, and routes them to the appropriate module callback function.

---

## Buttons Module

**Prefix:** `$buttons`

The Buttons module handles physical input. It provides software debouncing and allows you to bind any system command (or sequence of commands) to a physical button event (Press, Release, or Change).

| Command       | Description                                                                                                                                                                 | Sample Usage                                         |
| :------------ | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :--------------------------------------------------- |
| **`status`**  | Get module status.                                                                                                                                                          | `$buttons status`                                    |
| **`reset`**   | Reset the module.                                                                                                                                                           | `$buttons reset`                                     |
| **`enable`**  | Enable the Buttons module.                                                                                                                                                  | `$buttons enable`                                    |
| **`disable`** | Disable the Buttons module.                                                                                                                                                 | `$buttons disable`                                   |
| **`add`**     | Add a button mapping.<br>**Args:** `<pin> "<cmd>" [mode] [trigger] [debounce_ms]`<br>**Modes:** `pullup`, `pulldown`<br>**Triggers:** `on_press`, `on_release`, `on_change` | `$buttons add 9 "$system reboot" pullup on_press 50` |
| **`remove`**  | Remove a button mapping by pin number.                                                                                                                                      | `$buttons remove 9`                                  |

---

## Wifi Module

**Prefix:** `$wifi`

The Wifi module manages the ESP32's network connection. It handles connecting to credentials stored in NVS and scanning for available networks.

| Command          | Description                                            | Sample Usage       |
| :--------------- | :----------------------------------------------------- | :----------------- |
| **`status`**     | Get connection status and IP.                          | `$wifi status`     |
| **`reset`**      | Reset the module.                                      | `$wifi reset`      |
| **`enable`**     | Enable WiFi (starts radio).                            | `$wifi enable`     |
| **`disable`**    | Disable WiFi (saves power).                            | `$wifi disable`    |
| **`connect`**    | Attempt to connect/reconnect using stored credentials. | `$wifi connect`    |
| **`disconnect`** | Disconnect from current AP.                            | `$wifi disconnect` |
| **`scan`**       | List available WiFi networks (SSID/RSSI).              | `$wifi scan`       |

---

## Web Interface Module

**Prefix:** `$web_interface`

The Web Interface module spins up an HTTP server that allows other devices on the same network to send CLI commands to the XeWe OS via a web browser or API calls.

| Command       | Description                     | Sample Usage             |
| :------------ | :------------------------------ | :----------------------- |
| **`status`**  | Get server status.              | `$web_interface status`  |
| **`reset`**   | Reset the module.               | `$web_interface reset`   |
| **`enable`**  | Start the Web Interface server. | `$web_interface enable`  |
| **`disable`** | Stop the Web Interface server.  | `$web_interface disable` |

---

## Interface Module

**Internal Infrastructure**

The Interface module is the abstract base class for all external integration modules. It defines a unified synchronization contract:

* `sync_color(color_rgb)`
* `sync_brightness(brightness)`
* `sync_state(state)`
* `sync_mode(mode_id)`
* `sync_length(length)`
* optional `sync_all(color, brightness, state, mode, length)`

All integration modules (LED Strip, Web Interface, HomeKit, Alexa, NVS) inherit from this class to ensure consistent state propagation.

---

## LED Strip Module

**Prefix:** `$led`

The LED Strip module controls the physical addressable LED strip via FastLED. It supports RGB and HSV control, smooth transitions, global brightness, power state, selectable modes, and configurable strip length.

### Commands

| Command              | Description             | Sample Usage              |
| :------------------- | :---------------------- | :------------------------ |
| **`status`**         | Get module status.      | `$led status`             |
| **`reset`**          | Reset the module.       | `$led reset`              |
| **`set_rgb`**        | Set RGB color.          | `$led set_rgb 255 0 0`    |
| **`set_r`**          | Set red channel.        | `$led set_r 127`          |
| **`set_g`**          | Set green channel.      | `$led set_g 255`          |
| **`set_b`**          | Set blue channel.       | `$led set_b 200`          |
| **`set_hsv`**        | Set HSV color.          | `$led set_hsv 75 255 0`   |
| **`set_hue`**        | Set hue channel.        | `$led set_hue 255`        |
| **`set_sat`**        | Set saturation channel. | `$led set_sat 0`          |
| **`set_val`**        | Set value channel.      | `$led set_val 255`        |
| **`set_brightness`** | Set global brightness.  | `$led set_brightness 255` |
| **`set_state`**      | Set on/off state.       | `$led set_state 0`        |
| **`toggle_state`**   | If off→on, if on→off.   | `$led toggle_state`       |
| **`turn_on`**        | Turn strip on.          | `$led turn_on`            |
| **`turn_off`**       | Turn strip off.         | `$led turn_off`           |
| **`set_mode`**       | Set LED strip mode.     | `$led set_mode 0`         |
| **`set_length`**     | Set new number of LEDs. | `$led set_length 500`     |

---

## NVS Module

**Internal Infrastructure**

The Non-Volatile Storage (NVS) module persists LED state in ESP32 Preferences so settings survive reboots. It stores:

* `led_r`, `led_g`, `led_b`
* `led_bri`
* `led_state`
* `led_mode`
* `led_len`

This module has no CLI commands and cannot be disabled.

---

## Homekit Module

**Prefix:** `$homekit`

The HomeKit module exposes the LED strip to Apple Home via HomeSpan. It syncs power, brightness, and color by converting between internal RGB and HomeKit Hue/Saturation/Brightness.

**Notes**

* Requires Apple Hub (HomePod/Apple TV) for full Home App functionality as implemented.
* Reset requires removing the device from the Home App manually.

| Command       | Description          | Sample Usage       |
| :------------ | :------------------- | :----------------- |
| **`status`**  | Get module status.   | `$homekit status`  |
| **`reset`**   | Reset the module.    | `$homekit reset`   |
| **`enable`**  | Enable this module.  | `$homekit enable`  |
| **`disable`** | Disable this module. | `$homekit disable` |

---

## Alexa Module

**Prefix:** `$alexa`

The Alexa module integrates with Amazon Alexa via Espalexa. It exposes the device as a color-capable light and propagates Alexa state/brightness/color changes into the controller.

**Notes**

* Setup is discovery-based (“Ask Alexa to discover new devices”).
* Reset requires removing the device from the Alexa app manually.

| Command       | Description          | Sample Usage     |
| :------------ | :------------------- | :--------------- |
| **`status`**  | Get module status.   | `$alexa status`  |
| **`reset`**   | Reset the module.    | `$alexa reset`   |
| **`enable`**  | Enable this module.  | `$alexa enable`  |
| **`disable`** | Disable this module. | `$alexa disable` |
