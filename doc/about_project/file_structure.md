# Project Structure

```text
xewe-led-os/                                      # ESP32 firmware project
│
├── Config.h                                      # Project configuration file
├── xewe-led-os.ino                               # Arduino firmware entry
├── README.md                                     # Project overview documentation
├── LICENSE.txt                                   # Project license text
│
├── build/                                        # Build and tooling files
│   ├── libraries/                                # Build library configuration
│   │   └── required_libraries.txt                # Required Arduino libraries
│   │
│   ├── scripts/                                  # Platform build scripts
│   │   ├── mac/                                  # macOS build scripts
│   │   │   ├── build.sh                          # Build firmware script
│   │   │   ├── compile.sh                        # Compile firmware script
│   │   │   ├── format.sh                         # Format source script
│   │   │   ├── listen_serial.sh                  # Serial monitor script
│   │   │   ├── release.sh                        # Release firmware script
│   │   │   ├── setup_build_environment.sh        # Build environment setup
│   │   │   └── upload.sh                         # Upload firmware script
│   │   │
│   │   ├── linux/                                # Linux build scripts
│   │   └── windows/                              # Windows build scripts
│   │
│   └── tools/                                    # Build support tools
│       └── code_formatter/                       # Project source formatter
│
│
├── src/                                          # Firmware source code
│   ├── Utils/                                    # Shared utility helpers
│   │   ├── Debug.h                               # Debug logging helpers
│   │   ├── XeWeColor.h                           # Color utility type
│   │   ├── XeWeString.h                          # String utility helpers
│   │   ├── XeWeTimer.h                           # Timer utility helper
│   │   └── XeWeValidator.h                       # Validation utility helpers
│   │
│   └── Modules/                                  # Modular firmware features
│       ├── Module/                               # Base module framework
│       │   ├── Module.cpp                        # Base module implementation
│       │   ├── Module.h                          # Base module interface
│       │   ├── ModuleController.cpp              # Module controller implementation
│       │   ├── ModuleController.h                # Module controller interface
│       │   ├── SyncModule.cpp                    # Sync module implementation
│       │   └── SyncModule.h                      # Sync module interface
│       │
│       ├── Core/                                 # Core firmware services
│       │   ├── CommandExecutor/                  # Command execution service
│       │   ├── Nvs/                              # NVS storage service
│       │   ├── SerialPort/                       # Serial communication service
│       │   └── System/                           # System control service
│       │
│       ├── Hardware/                             # Hardware control modules
│       │   ├── Buttons/                          # Button input module
│       │   │
│       │   └── LedStrip/                         # LED strip module
│       │       ├── Brightness/                   # Brightness control module
│       │       └── ModeController/               # LED mode controller
│       │           ├── ModeRegistry/             # Mode registration service
│       │           └── Modes/                    # LED animation modes
│       │               ├── Mode/                 # Base mode framework
│       │               ├── ChristmasLights/      # Christmas lights mode
│       │               ├── FadeBrightness/       # Brightness fade mode
│       │               ├── FadeColor/            # Color fade mode
│       │               ├── FadeColorTwoZone/     # Two-zone fade mode
│       │               ├── Pulse/                # Pulse animation mode
│       │               ├── Rainbow/              # Rainbow animation mode
│       │               └── Solid/                # Solid color mode
│       │
│       └── Software/                             # Software service modules
│           ├── Wifi/                             # Wi-Fi connectivity service
│           │
│           ├── Time/                             # Time management service
│           │   └── Scheduler/                    # Scheduled task service
│           │
│           └── SmartHome/                        # Smart-home integrations
│               ├── Alexa/                        # Alexa integration module
│               ├── HomeAssistant/                # Home Assistant integration
│               ├── HomeKit/                      # HomeKit integration module
│               └── WebInterface/                 # Web control interface
│                   ├── WebInterface.cpp          # Web interface implementation
│                   ├── WebInterface.h            # Web interface declaration
│                   ├── static/                   # Embedded web resources
│                   └── templates/                # Embedded HTML templates
│
└── static/                                       # Releases, tests, media
    └── firmware/                                 # Firmware release files
        └── releases/                             # Versioned firmware releases
            └── <version>/                        # Target firmware packages
```
