# Code Quality Report

The project contains both carefully reviewed code and AI-assisted code. This document tracks the current review status.
Use this report as a guide for future code quality improvements.

| Label | Meaning |
|---|---|
| 🟢 high | A human rigorously reviewed the code logic. |
| 🟡 mid | The code combines human logic and AI implementation. |
| 🔴 low | AI generated the code, and it needs careful review. |

| File                                                           | Code quality   |
|----------------------------------------------------------------|----------------|
| `src/Utils/Debug.h`                                            | 🟢 high        |
| `src/Utils/XeWeColor.h`                                        | 🟢 high        |
| `src/Utils/XeWeString.h`                                       | 🟡 mid         |
| `src/Utils/XeWeTimer.h`                                        | 🟢 high        |
| `src/Utils/XeWeValidator.h`                                    | 🟡 mid         |
| -------------------------------------------------------------- | -------------- |
| `src/Modules/Module/Module`                                    | 🟢 high        |
| `src/Modules/Module/ModuleController`                          | 🟢 high        |
| `src/Modules/Module/SyncModule`                                | 🟢 high        |
| -------------------------------------------------------------- | -------------- |
| `src/Modules/Core/CommandExecutor`                             | 🟢 high        |
| `src/Modules/Core/Nvs`                                         | 🟢 high        |
| `src/Modules/Core/SerialPort`                                  | 🟢 high        |
| `src/Modules/Core/System`                                      | 🟢 high        |
| -------------------------------------------------------------- | -------------- |
| `src/Modules/Hardware/Buttons`                                 | 🟡 mid         |
| `src/Modules/Hardware/LedStrip/LedStrip`                       | 🟡 mid         |
| `src/Modules/Hardware/LedStrip/Brightness`                     | 🟡 mid         |
| `src/Modules/Hardware/LedStrip/ModeController`                 | 🟡 mid         |
| -------------------------------------------------------------- | -------------- |
| `src/Modules/Software/Wifi`                                    | 🟢 high        |
| `src/Modules/Software/Time`                                    | 🟢 high        |
| `src/Modules/Software/Time/Scheduler`                          | 🟢 high        |
| `src/Modules/Software/SmartHome/Alexa`                         | 🟢 high        |
| `src/Modules/Software/SmartHome/HomeAssistant`                 | 🟡 mid         |
| `src/Modules/Software/SmartHome/HomeKit`                       | 🟢 high        |
| `src/Modules/Software/SmartHome/WebInterface`                  | 🟡 mid         |
