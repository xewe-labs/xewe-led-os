# Module & Command Reference

XeWe OS uses independent modules. This reference describes the current module architecture and the documented CLI commands.

## Module Framework

### Module

**Internal Infrastructure**

`Module` is the base class for firmware modules. It provides lifecycle control, dependency support, status functions, and optional CLI commands.

Common lifecycle functions include:

* `begin_routines_required()`
* `begin_routines_init()`
* `begin_routines_regular()`
* `begin_routines_common()`
* `loop()`
* `enable()`
* `disable()`
* `reset()`
* `status()`

### ModuleController

**Internal Infrastructure**

`ModuleController` owns the firmware modules and controls their initialization. Modules use its shared services to access other modules.

### SyncModule

**Internal Infrastructure**

`SyncModule` inherits from `Module`. It defines the common state synchronization contract for modules that share LED state.

Required synchronization functions:

* `sync_color(color)`
* `sync_brightness(brightness)`
* `sync_state(state)`
* `sync_mode(mode)`
* `sync_length(length)`

`sync_all(color, brightness, state, mode, length)` calls the required synchronization functions for one full state update.

`sync_param(key, value)` provides optional synchronization for mode parameters. The default implementation does nothing.

---

## Core Modules

### SerialPort Module

**Internal Infrastructure**

The SerialPort module handles UART communication. It provides non-blocking input, input buffers, CLI output, and debug output.

---

### System Module

**Prefix:** `$system`

The System module provides system control and hardware information. It also provides restart, identification, and task stack information.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the System module status. | `$system status` |
| **`reset`** | Reset the System module logic. | `$system reset` |
| **`restart`** | Restart the ESP32. | `$system restart` |
| **`reboot`** | Run the restart command. | `$system reboot` |
| **`info`** | Show chip and build information. | `$system info` |
| **`mac`** | Show the device MAC addresses. | `$system mac` |
| **`uid`** | Show the device UID. | `$system uid` |
| **`stack`** | Show the task stack watermark. | `$system stack` |

---

### CommandExecutor Module

**Internal Infrastructure**

The CommandExecutor module processes CLI command input. It routes commands to the applicable module callback.

---

### NVS Module

**Internal Infrastructure**

The NVS module provides non-volatile key-value storage for other modules. Modules use it to store persistent state and configuration data.

The NVS module also provides typed storage helpers. It has no documented user CLI commands.

---

## Hardware Modules

### Buttons Module

**Prefix:** `$buttons`

The Buttons module handles physical button input. It provides software debounce and maps button events to system commands.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the module status. | `$buttons status` |
| **`reset`** | Reset the module. | `$buttons reset` |
| **`enable`** | Enable the Buttons module. | `$buttons enable` |
| **`disable`** | Disable the Buttons module. | `$buttons disable` |
| **`add`** | Add a button mapping.<br>**Args:** `<pin> "<cmd>" [mode] [trigger] [debounce_ms]`<br>**Modes:** `pullup`, `pulldown`<br>**Triggers:** `on_press`, `on_release`, `on_change` | `$buttons add 9 "$system reboot" pullup on_press 50` |
| **`remove`** | Remove a button mapping by pin. | `$buttons remove 9` |

---

### LED Strip Module

**Prefix:** `$led`

The LED Strip module controls the physical addressable LED strip. It provides color, brightness, power, mode, and strip length control.

The LED Strip module contains `Brightness` and `ModeController` components. `ModeController` uses `ModeRegistry` and the available LED modes.

Available modes:

* `ChristmasLights`
* `FadeBrightness`
* `FadeColor`
* `FadeColorTwoZone`
* `Pulse`
* `Rainbow`
* `Solid`

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the module status. | `$led status` |
| **`reset`** | Reset the module. | `$led reset` |
| **`set_rgb`** | Set the RGB color. | `$led set_rgb 255 0 0` |
| **`set_r`** | Set the red channel. | `$led set_r 127` |
| **`set_g`** | Set the green channel. | `$led set_g 255` |
| **`set_b`** | Set the blue channel. | `$led set_b 200` |
| **`set_hsv`** | Set the HSV color. | `$led set_hsv 75 255 0` |
| **`set_hue`** | Set the hue channel. | `$led set_hue 255` |
| **`set_sat`** | Set the saturation channel. | `$led set_sat 0` |
| **`set_val`** | Set the value channel. | `$led set_val 255` |
| **`set_brightness`** | Set the global brightness. | `$led set_brightness 255` |
| **`set_state`** | Set the power state. | `$led set_state 0` |
| **`toggle_state`** | Toggle the power state. | `$led toggle_state` |
| **`turn_on`** | Turn the strip on. | `$led turn_on` |
| **`turn_off`** | Turn the strip off. | `$led turn_off` |
| **`set_mode`** | Set the LED mode. | `$led set_mode 0` |
| **`set_length`** | Set the LED count. | `$led set_length 500` |

---

## Software Modules

### Wifi Module

**Prefix:** `$wifi`

The Wifi module controls the ESP32 network connection. It uses stored credentials and scans for available networks.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the connection status and IP address. | `$wifi status` |
| **`reset`** | Reset the module. | `$wifi reset` |
| **`enable`** | Enable WiFi. | `$wifi enable` |
| **`disable`** | Disable WiFi. | `$wifi disable` |
| **`connect`** | Connect with stored credentials. | `$wifi connect` |
| **`disconnect`** | Disconnect from the current access point. | `$wifi disconnect` |
| **`scan`** | List available WiFi networks. | `$wifi scan` |

---

### Time Module

The Time module provides time services for the firmware. The `Scheduler` component provides scheduled task support.

No CLI command list is documented in this reference.

---

## Smart Home Modules

### Web Interface Module

**Prefix:** `$web_interface`

The Web Interface module provides HTTP access to firmware controls. It serves the embedded web resources and templates.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the server status. | `$web_interface status` |
| **`reset`** | Reset the module. | `$web_interface reset` |
| **`enable`** | Start the Web Interface server. | `$web_interface enable` |
| **`disable`** | Stop the Web Interface server. | `$web_interface disable` |

---

### Home Assistant Module

The Home Assistant module provides the Home Assistant integration. Its source is in `src/Modules/Software/SmartHome/HomeAssistant/`.

No CLI command list is documented in this reference.

---

### HomeKit Module

**Prefix:** `$homekit`

The HomeKit module exposes the LED strip to Apple Home through HomeSpan. It synchronizes power, brightness, and color state.

**Notes**

* Full Home app functionality requires an Apple home hub for the current implementation.
* A reset requires manual device removal from the Home app.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the module status. | `$homekit status` |
| **`reset`** | Reset the module. | `$homekit reset` |
| **`enable`** | Enable the module. | `$homekit enable` |
| **`disable`** | Disable the module. | `$homekit disable` |

---

### Alexa Module

**Prefix:** `$alexa`

The Alexa module integrates with Amazon Alexa through Espalexa. It synchronizes Alexa power, brightness, and color changes with the firmware.

**Notes**

* Setup uses Alexa device discovery.
* A reset requires manual device removal from the Alexa app.

| Command | Description | Sample Usage |
| :--- | :--- | :--- |
| **`status`** | Get the module status. | `$alexa status` |
| **`reset`** | Reset the module. | `$alexa reset` |
| **`enable`** | Enable the module. | `$alexa enable` |
| **`disable`** | Disable the module. | `$alexa disable` |
