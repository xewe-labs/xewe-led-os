//#include "ControllerInterfaceTemplate.h"
//
//// --- Define the module's unique memory key and command list ---
//
//const String ControllerInterfaceTemplate::MODULE_MEM_KEY = "interfaceTemplate";
//const size_t ControllerInterfaceTemplate::NUM_COMMANDS = 2; // IMPORTANT: Must match the number of commands below
//const CommandParser::Command ControllerInterfaceTemplate::COMMANDS[ControllerInterfaceTemplate::NUM_COMMANDS] = {
//    // { "commandName", METHOD_CALLBACK(&ControllerInterfaceTemplate::commandHandler) },
//    // Example placeholder:
//    { "ping", nullptr },
//    { "reboot", nullptr }
//};
//
//
//// --- Constructor and Destructor ---
//
//ControllerInterfaceTemplate::ControllerInterfaceTemplate(SystemController& controller_ref)
//    : ControllerModule(controller_ref), ControllerInterface(controller_ref) {
//    // TODO: Initialize member variables with default values if needed
//    is_on = false;
//    current_brightness = 255;
//    current_color = {255, 255, 255};
//}
//
//ControllerInterfaceTemplate::~ControllerInterfaceTemplate() {
//    // TODO: Clean up any resources allocated in begin()
//}
//
//
//// --- ControllerModule Implementation ---
//
//bool ControllerInterfaceTemplate::begin(void* context, const String& device_name) {
//    // TODO: Initialization code goes here.
//    return true;
//}
//
//bool ControllerInterfaceTemplate::loop() {
//    // TODO: Main logic goes here.
//    return true;
//}
//
//bool ControllerInterfaceTemplate::reset() {
//    // TODO: Code to reset the module to its default state.
//    return true;
//}
//
//bool ControllerInterfaceTemplate::status() {
//    // TODO: Code to report the module's status.
//    return true;
//}
//
//bool ControllerInterfaceTemplate::enable() {
//    // TODO: Code to enable the module's operation.
//    is_on = true;
//    return true;
//}
//
//bool ControllerInterfaceTemplate::disable() {
//    // TODO: Code to disable the module's operation.
//    is_on = false;
//    return true;
//}
//
//// --- Protected Getters for Module ---
//
//const CommandParser::Command* ControllerInterfaceTemplate::get_commands() const {
//    return ControllerInterfaceTemplate::COMMANDS;
//}
//
//size_t ControllerInterfaceTemplate::get_commands_num() const {
//    return ControllerInterfaceTemplate::NUM_COMMANDS;
//}
//
//const String& ControllerInterfaceTemplate::get_mem_key() const {
//    return ControllerInterfaceTemplate::MODULE_MEM_KEY;
//}
//
//
//// --- ControllerInterface Implementation ---
//
//void ControllerInterfaceTemplate::sync_color(std::array<uint8_t, 3> color) {
//    // TODO: Act on the synchronized color data.
//    this->current_color = color;
//}
//
//void ControllerInterfaceTemplate::sync_brightness(uint8_t brightness) {
//    // TODO: Act on the synchronized brightness data.
//    this->current_brightness = brightness;
//}
//
//void ControllerInterfaceTemplate::sync_state(bool state) {
//    // TODO: Act on the synchronized state data (on/off).
//    this->is_on = state;
//}
//
//void ControllerInterfaceTemplate::sync_mode(uint8_t mode_id, String mode_name) {
//    // TODO: Act on the synchronized mode data.
//}
//
//void ControllerInterfaceTemplate::sync_length(uint16_t length) {
//    // TODO: Act on the synchronized length data.
//}
//
//void ControllerInterfaceTemplate::sync_all(std::array<uint8_t, 3> color, uint8_t brightness, bool state, uint8_t mode_id, String mode_name, uint16_t length) {
//    // TODO: A convenience function to sync all properties at once.
//    sync_color(color);
//    sync_brightness(brightness);
//    sync_state(state);
//    sync_mode(mode_id, mode_name);
//    sync_length(length);
//}