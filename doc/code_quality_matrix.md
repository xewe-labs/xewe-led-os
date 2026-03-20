# Code Quality Report

Since the project is a mix of well thought-through engineering decisions ans vibecoding, this document is meant to track it.
This serves as a guide for the future code quality improvement.

| Label   | Meaning                                                |
|---------|--------------------------------------------------------|
| 🟢 high | code logic was rigorously reviewed by a human         |
| 🟡 mid  | code is a mix of human logic and ai implementation     |
| 🔴 low  | code is completely ai generated and may be very sloppy |

| File                                                                                        | Code quality |
|---------------------------------------------------------------------------------------------| -- |
| `./Debug.h`                                                                                 | 🟢 high |
| `./XeWeColorUtils.h`                                                                        | 🟢 high |
| `./XeWeStringUtils.h`                                                                       | 🟡 mid |
| `./Modules/Hardware/Buttons/Buttons.cpp`                                                    | 🟡 mid |
| `./Modules/Hardware/Buttons/Buttons.h`                                                      | 🟡 mid |
| `./Modules/Module/Module.cpp`                                                               | 🟢 high |
| `./Modules/Module/Module.h`                                                                 | 🟢 high |
| `./Modules/Software/CommandParser/CommandParser.cpp`                                        | 🟡 mid |
| `./Modules/Software/CommandParser/CommandParser.h`                                          | 🟡 mid |
| `./Modules/Software/Scheduler/Scheduler.cpp`                                                | 🔴 low |
| `./Modules/Software/Scheduler/Scheduler.h`                                                  | 🔴 low |
| `./Modules/Software/SerialPort/SerialPort.cpp`                                              | 🟢 high |
| `./Modules/Software/SerialPort/SerialPort.h`                                                | 🟢 high |
| `./Modules/Software/System/System.cpp`                                                      | 🟢 high |
| `./Modules/Software/System/System.h`                                                        | 🟢 high |
| `./Modules/Software/Time/Time.cpp`                                                          | 🟡 mid |
| `./Modules/Software/Time/Time.h`                                                            | 🟡 mid |
| `./Modules/Software/Wifi/Wifi.cpp`                                                          | 🟢 high |
| `./Modules/Software/Wifi/Wifi.h`                                                            | 🟢 high |
| `./SystemController/SystemController.cpp`                                                   | 🟢 high |
| `./SystemController/SystemController.h`                                                     | 🟢 high |
| `./Interfaces/Interface/Interface.h`                                                        | 🟢 high |
| `./Interfaces/Interface/Interface.cpp`                                                      | 🟢 high |
| `./Interfaces/Hardware/LedStrip/AsyncTimer/AsyncTimer.h`                                    | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/Brightness/Brightness.cpp`                                  | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/Brightness/Brightness.h`                                    | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Mode/Mode.h`                           | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Mode/Mode.cpp`                         | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColorTwoZone/FadeColorTwoZone.h`   | 🔴 low |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColorTwoZone/FadeColorTwoZone.cpp` | 🔴 low |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.h`                 | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeColor/FadeColor.cpp`               | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.cpp`                       | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Pulse/Pulse.h`                         | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Solid/Solid.h`                         | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Solid/Solid.cpp`                       | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.cpp`                   | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/Rainbow/Rainbow.h`                     | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.cpp`     | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/FadeBrightness/FadeBrightness.h`       | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/ChristmasLights/ChristmasLights.h`     | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/Modes/ChristmasLights/ChristmasLights.cpp`   | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/ModeRegistry/ModeRegistry.h`                 | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/ModeController.h`                            | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/ModeController/ModeController.cpp`                          | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/LedStrip.h`                                                 | 🟡 mid |
| `./Interfaces/Hardware/LedStrip/LedStrip.cpp`                                               | 🟡 mid |
| `./Interfaces/Software/HomeKit/HomeKit.cpp`                                                 | 🟡 mid |
| `./Interfaces/Software/HomeKit/HomeKit.h`                                                   | 🟡 mid |
| `./Interfaces/Software/WebInterface/index.html.h`                                           | 🟡 mid |
| `./Interfaces/Software/WebInterface/WebInterface.cpp`                                       | 🔴 low |
| `./Interfaces/Software/WebInterface/index.js.h`                                             | 🔴 low |
| `./Interfaces/Software/WebInterface/index.css.h`                                            | 🔴 low |
| `./Interfaces/Software/WebInterface/WebInterface.h`                                         | 🔴 low |
| `./Interfaces/Software/Alexa/Alexa.h`                                                       | 🟡 mid |
| `./Interfaces/Software/Alexa/Alexa.cpp`                                                     | 🟡 mid |
| `./Interfaces/Software/Nvs/Nvs.h`                                                           | 🟢 high |
| `./Interfaces/Software/Nvs/Nvs.cpp`                                                         | 🟢 high |
