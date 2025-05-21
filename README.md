# Welcome to XeWe Led OS

## Quickstart

### Download Arduino IDE
  - For Mac users, you need to install the **Intel** version, **APPLE SILICON** VERSION DOES NOT WORK WITH ESP32 BOARDS
  - Launch IDE
  - Go to settings
  - Set Additional boards manager URLs: http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - <img width="700" alt="Screenshot 2025-05-21 at 10 46 43" src="https://github.com/user-attachments/assets/a448ee9f-3980-45c7-85a8-f5dc5eba1370" />
  - Close IDE

### Connect ESP32 C3 to USB

### Open [XeWe-LedOS.ino](XeWe-LedOS.ino) sketch
  - Select board ESP32C3 Dev Module
  - Select your port
  - <img width="700" alt="Screenshot 2025-05-21 at 09 44 20" src="https://github.com/user-attachments/assets/d61a7f68-150b-4fed-907f-825b116874f3" />
  - Go to Tools -> USB CDC On Boot -> "Enabled"
  - <img width="700" alt="Screenshot 2025-05-21 at 09 54 52" src="https://github.com/user-attachments/assets/ee627ead-79bf-4b7e-879d-478cce3d538e" />
  - In the sketch:
    - Make sure you have ```#define LED_PIN <your_led_strip_pin>```
    - Verify ```#define NUM_LEDS <your_led_strip_led_count>```
  - Press upload

### To communicate with the board, you can use Serial Port
  - Go to Tools -> Serial Monitor
  - Set 115200baud rate
  - Disconnect and connect the board to reload it

### Connect to WiFi
  - WiFi connection will be prompted automatically
  - Select your WiFi network using the number
  - Enter password
  - Done!

### Command Line Tool Control
  - Allows you to control the ESP32 with commands from the Serial Port
    - Commands must follow the structure: ```$<cmd_group> <cmd_name> <<param_0> <param_1> ... <param_n>>```
    - Parameters have the range 0-255: ```$led set_brightness <0-255>```
    - Parameters must be separated with a space: ```$led set_rgb <0-255> <0-255> <0-255>```
  - To see all commands available type ```$help```
  - To see wifi commands available type ```$wifi help```
  - To see led commands available type ```$led help```

### Control LED Strip
  - IMPORTANT: If it is your first startup type ```$led reset``` to reset the EEPROM memory; you will see pale green color 
  - Example: 
    - Set led to red: ```$led set_rgb 255 0 0```
    - Set brightness to 50%: ```$led set_brightness 127```
    - Turn on: ```$led turn_on```
