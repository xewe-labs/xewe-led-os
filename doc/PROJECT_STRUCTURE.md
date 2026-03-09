```text
xewe-os/                                           # Project root (ESP32 “xewe-os” firmware + tooling)
│
├── build/                                         # Build tooling + build artifacts
│   ├── releases/                                  
│   ├── builds/                                    
│   │   ├── .version_state                         # Tracks version/build state used by scripts (bump/last build info)
│   │   ├── latest/                                # Pointer to most recent build output
│   │   ├── cache/                                 # Compiler/toolchain cache
│   │   └── DATETIME-VERSION-ESP32-CHIP-xewe-os/   # One build “snapshot” (logs, binaries, merged images, copied src)
│   │
│   └── scripts/                                   
│       ├── build.sh                               # Orchestrates full build pipeline (compile + upload + push to git + listen port)
│       ├── compile.sh                             # Performs compilation of the src
│       ├── listen_serial.sh                       # Monitor serial port
│       ├── push_to_git.sh                         # Helper to commit/push .bin firmware to binaries branch
│       ├── release.sh                             # Builds and pushes for multiple targets, bumps .version_state
│       └── setup_build_enviroment.sh              # Installs/sets env vars/tools needed to build
│
├── doc/                                           
├── ├── src_templates/                                 # Starter templates for generating new modules/files
│       ├── ModuleTemplate.cpp                         # Example implementation skeleton
│       └── ModuleTemplate.h                           # Example header skeleton
│   ├── ADDING_A_MODULE_OR_INTERFACE.md            # How to create/register a new module in this architecture
│   ├── CONTRIBUTING.md                            # Contribution rules (style, workflow, PR expectations)
│   ├── MODULES_AND_INTERFACES.md                  # Contribution rules (style, workflow, PR expectations)
│   ├── PROJECT_STRUCTURE.md                       # High-level layout and responsibilities of folders/modules
│   └── license_header.txt                         # Standard license header text to paste into new files
│
├── src/                                          
│   ├── Debug.h                                    # Debug/logging macros, flags, and helpers
│   ├── XeWeStringUtils.h                          # Shared string utilities
│   ├── build_info.h                               # Build metadata
│   ├── Modules/                                   # Modular feature units
│   │   ├── Hardware/                              # Modules that touch GPIO/peripherals directly
│   │   │   └── Buttons/                           # Button input handling (read/debounce/events)
│   │   │       ├── Buttons.cpp                    
│   │   │       └── Buttons.h                      
│   │   ├── Module/                                # Base module framework
│   │   │   ├── Module.cpp                         
│   │   │   └── Module.h                           
│   │   └── Software/                              # Modules providing higher-level services
│   │       ├── CommandParser/                     # Parses commands (CLI/serial/web commands → actions)
│   │       │   ├── CommandParser.cpp              
│   │       │   └── CommandParser.h                
│   │       ├── Nvs/                               # Non-volatile storage wrapper (ESP32 NVS key/value)
│   │       │   ├── Nvs.cpp                        
│   │       │   └── Nvs.h                          
│   │       ├── SerialPort/                        # Serial I/O abstraction with handy methods
│   │       │   ├── SerialPort.cpp                 
│   │       │   └── SerialPort.h                   
│   │       ├── System/                            # System services (state, timing, restart, metrics, etc.)
│   │       │   ├── System.cpp                     
│   │       │   └── System.h                       
│   │       ├── WebInterface/                      # HTTP/web UI endpoints + handlers
│   │       │   ├── WebInterface.cpp               
│   │       │   └── WebInterface.h                 
│   │       └── Wifi/                              # Wi-Fi connectivity management (join/AP, reconnect, status)
│   │           ├── Wifi.cpp                       
│   │           └── Wifi.h                         
│   └── SystemController/                          # Application orchestrator
│       ├── SystemController.cpp                   
│       └── SystemController.h                     
│
│
└── static/                                        # Static assets (images/files served by web UI or used in docs)
```