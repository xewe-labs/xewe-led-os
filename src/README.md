# src/

This folder holds the product's own modules, one folder per module, each an `xewe::Module` declared in `xewe-led-os.ino`. The generated, git-ignored `src/Modules.h` declares the modules selected from xewe-os-modules.

| Folder | What it adds |
|---|---|
| [SchedulePage/](SchedulePage/README.md) | the schedule editor page and its JSON routes |
| [HomeAssistant/](HomeAssistant/README.md) | Home Assistant over MQTT auto-discovery |
| [HomeKit/](HomeKit/README.md) | Apple HomeKit through HomeSpan |
| [Alexa/](Alexa/README.md) | Alexa through Espalexa |

Each README lists the module's id, commands, first-boot questions and tests. The sketch declares the four in the order of this table, and its comment gives the reason for each position. [doc/WRITING_A_MODULE.md](../doc/WRITING_A_MODULE.md) explains the module contract.
