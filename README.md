# XeWe Led OS
### The ultimate LED Strip Software for ESP32

## The problem
I have built many LED applications. In-between them I had a lot of repetitive work that I decided to distill in one piece of software-- Led OS  
Features:
- CLI commands via Serial Port to control addressable LED strip
- WiFi connectivity that allows
    - Local Web Server for control via the web browser on the same network
    - Alexa voice + app control (requires Alexa Speaker)
    - Apple HomeKit + Siri control (requires hub: Apple TV or Speaker)
- Physical buttons support
- CLI commands via Serial Port for dynamic configuration

## Supported Hardware
- ESP32 C3, ESP32 C6, ESP32 S3
![IMG_2737.jpeg](static/media/resources/readme/IMG_2737.jpeg)

## Quickstart
### Easy Way
Upload precompiled software from the website.
Go to https://maxdokukin.com/projects/xewe-led-os
![Screenshot 2026-01-17 at 09.44.48.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.44.48.webp)
Select the port
![Screenshot 2026-01-17 at 09.49.05.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.49.05.webp)
Click install
![Screenshot 2026-01-17 at 09.52.39.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.52.39.webp)
After installation finishes, got to "Logs & Console"
![Screenshot 2026-01-17 at 09.53.53.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.53.53.webp)
Click "Reset Device", this will reboot the board
![Screenshot 2026-01-17 at 09.54.50.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.54.50.webp)
Finish by following the Serial Port instructions  
**NOTE: that sometimes a line of text can go missing.   
If next step makes no sense, hit "Enter"  
To avoid this issue, use a more robust Serial Port monitor app at 115200baud**
![Screenshot 2026-01-17 at 09.55.34.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2009.55.34.webp)
You will see "Rebooting..." at the end of the setup
![Screenshot 2026-01-17 at 10.00.07.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2010.00.07.webp)
Done. Try $help to see all commands available
![Screenshot 2026-01-17 at 10.03.41.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2010.03.41.webp)

### Technical Way
Compile the software yourself using Arduino IDE, or provided script.

#### Arduino IDE
- Set up IDE for ESP32 development; sample guide https://www.espboards.dev/blog/setting-up-arduino-ide-esp32/
  - Common issue: https://forum.arduino.cc/t/downloading-esp32-3-3-5-fails/1420739

- Download libraries:
  - "FastLED" https://github.com/FastLED/FastLED
  - "Espalexa" (modified, use Github) https://github.com/maxdokukin/xewe-led-espalexa
  - "HomeSpan" https://github.com/HomeSpan/HomeSpan
  - "WebSockets" https://github.com/Links2004/arduinoWebSockets

**Note that on Mac with Apple Silicon, you need Arduino IDE Intel edition + Rosetta.
Otherwise, the sketch won't compile or will cause a core dump.**

Ensure that your config matches:
![Screenshot 2026-01-17 at 12.00.04.webp](static/media/resources/readme/Screenshot%202026-01-17%20at%2012.00.04.webp)

#### Scripts (the way I do it)
Will only work on Mac/Linux. 

- Install Arduino CLI
- Use compile.sh

## About the Features:
### CLI Tool
This is a handy way to have a high level control over the LEDs.
You can send multiple commands to configure the state of LEDs.
You can also use another devices to automatically send the commands via serial port.

Commands must follow the structure: $<cmd_group> <cmd_name> <<param_0> <param_1> ... <param_n>>   
Parameters have the range 0-255: $led set_brightness <0-255>   
Parameters must be separated with a space: $led set_rgb <0-255> <0-255> <0-255>   

- To see all commands available type $help
- To see system commands available type $system help
- To see wifi commands available type $wifi help
- To see led commands available type $led help

### Web Interface UI

### Apple Homekit Support

### Alexa

### Buttons



## Software Modularity
Modules (WiFi, Homekit, ...) can all be enabled or disabled dynamically based on your needs
For example, if you decided to stop using wifi, type $wifi disable. Alternatively, you can bring a module back by typing $<module> enable.

The list of commands that all modules support:
- $<module> status
- $<module> reset
Some modules can be toggled:
- $<module> enable
- $<module> disable
