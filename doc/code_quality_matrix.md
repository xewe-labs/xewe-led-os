# Code Quality Report

The project contains both carefully reviewed code and AI-assisted code. This document tracks the current review status.
Use this report as a guide for future code quality improvements.

| Label | Meaning |
|---|---|
| 🟢 high | A human rigorously reviewed the code logic. |
| 🟡 mid | The code combines human logic and AI implementation. |
| 🔴 low | AI generated the code, and it needs careful review. |
| ⚪ unrated | The previous matrix did not contain this file. |

| File | Code quality |
|---|---|
| `./src/Utils/Debug.h` | 🟢 high |
| `./src/Utils/XeWeColor.h` | 🟢 high |
| `./src/Utils/XeWeString.h` | 🟡 mid |
| `./src/Utils/XeWeTimer.h` | ⚪ unrated |
| `./src/Utils/XeWeValidator.h` | ⚪ unrated |
| `./src/Modules/Module/Module.cpp` | 🟢 high |
| `./src/Modules/Module/Module.h` | 🟢 high |
| `./src/Modules/Module/ModuleController.cpp` | ⚪ unrated |
| `./src/Modules/Module/ModuleController.h` | ⚪ unrated |
| `./src/Modules/Module/SyncModule.cpp` | ⚪ unrated |
| `./src/Modules/Module/SyncModule.h` | ⚪ unrated |
| `./src/Modules/Core/CommandExecutor/CommandExecutor.cpp` | ⚪ unrated |
| `./src/Modules/Core/CommandExecutor/CommandExecutor.h` | ⚪ unrated |
| `./src/Modules/Core/Nvs/FlexData.h` | ⚪ unrated |
| `./src/Modules/Core/Nvs/Nvs.cpp` | 🟢 high |
| `./src/Modules/Core/Nvs/Nvs.h` | 🟢 high |
| `./src/Modules/Core/Nvs/Nvs.tpp` | ⚪ unrated |
| `./src/Modules/Core/SerialPort/SerialPort.cpp` | 🟢 high |
| `./src/Modules/Core/SerialPort/SerialPort.h` | 🟢 high |
| `./src/Modules/Core/System/System.cpp` | 🟢 high |
| `./src/Modules/Core/System/System.h` | 🟢 high |
| `./src/Modules/Hardware/Buttons/Buttons.cpp` | 🟡 mid |
| `./src/Modules/Hardware/Buttons/Buttons.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/LedStrip.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/LedStrip.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/Brightness/Brightness.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/Brightness/Brightness.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/ModeController.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/ModeController.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/ModeRegistry/ModeRegistry.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Mode/Mode.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Mode/Mode.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/ChristmasLights/ChristmasLights.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/ChristmasLights/ChristmasLights.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColorTwoZone/FadeColorTwoZone.cpp` | 🔴 low |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/FadeColorTwoZone/FadeColorTwoZone.h` | 🔴 low |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.h` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Solid/Solid.cpp` | 🟡 mid |
| `./src/Modules/Hardware/LedStrip/ModeController/Modes/Solid/Solid.h` | 🟡 mid |
| `./src/Modules/Software/Wifi/Wifi.cpp` | 🟢 high |
| `./src/Modules/Software/Wifi/Wifi.h` | 🟢 high |
| `./src/Modules/Software/Time/Time.cpp` | 🟡 mid |
| `./src/Modules/Software/Time/Time.h` | 🟡 mid |
| `./src/Modules/Software/Time/Scheduler/Scheduler.cpp` | 🔴 low |
| `./src/Modules/Software/Time/Scheduler/Scheduler.h` | 🔴 low |
| `./src/Modules/Software/SmartHome/Alexa/Alexa.cpp` | 🟡 mid |
| `./src/Modules/Software/SmartHome/Alexa/Alexa.h` | 🟡 mid |
| `./src/Modules/Software/SmartHome/HomeAssistant/HomeAssistant.cpp` | ⚪ unrated |
| `./src/Modules/Software/SmartHome/HomeAssistant/HomeAssistant.h` | ⚪ unrated |
| `./src/Modules/Software/SmartHome/HomeKit/HomeKit.cpp` | 🟡 mid |
| `./src/Modules/Software/SmartHome/HomeKit/HomeKit.h` | 🟡 mid |
| `./src/Modules/Software/SmartHome/WebInterface/WebInterface.cpp` | 🔴 low |
| `./src/Modules/Software/SmartHome/WebInterface/WebInterface.h` | 🔴 low |
