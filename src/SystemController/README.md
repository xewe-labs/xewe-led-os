# System Controller

## Purpose
- ensure system operating
- smooth init and shutdown
- sync between input (serial_port, web_interface, alexa, homekit), storage (nvs) and display (led_strip)
- dynamic modularity ontrol
- run all the active modules
- provide execution env for the cmd parser for user control

## Content
- SystemController - main system controller (aka CEO that manages everything)
- ControllerModule - template (required functions) for the modules that want to control LED strip
- CommandParser - stores all the cmd available, maps Serial Port input onto actual functions