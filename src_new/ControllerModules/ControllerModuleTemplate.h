//#include "ControllerModuleTemplate.h"
//
//// --- Define the module's unique memory key and command list ---
//
//const String ControllerModuleTemplate::MODULE_MEM_KEY = "moduleTemplate";
//const size_t ControllerModuleTemplate::NUM_COMMANDS = 1; // IMPORTANT: Must match the number of commands below
//const CommandParser::Command ControllerModuleTemplate::COMMANDS[ControllerModuleTemplate::NUM_COMMANDS] = {
//    // { "commandName", METHOD_CALLBACK(&ControllerModuleTemplate::commandHandler) },
//    // Example placeholder:
//    { "report", nullptr }
//};
//
//
//// --- Constructor and Destructor ---
//
//ControllerModuleTemplate::ControllerModuleTemplate(SystemController& controller_ref)
//    : ControllerModule(controller_ref) {
//    loop_counter = 0;
//}
//
//ControllerModuleTemplate::~ControllerModuleTemplate() {
//    // TODO: Clean up any resources
//}
//
//
//// --- ControllerModule Implementation ---
//
//bool ControllerModuleTemplate::begin(void* context, const String& device_name) {
//    // TODO: Initialization code goes here.
//    return true;
//}
//
//bool ControllerModuleTemplate::loop() {
//    // TODO: Main logic goes here.
//    loop_counter++;
//    return true;
//}
//
//bool ControllerModuleTemplate::reset() {
//    // TODO: Code to reset the module to its default state.
//    loop_counter = 0;
//    return true;
//}
//
//bool ControllerModuleTemplate::status() {
//    // TODO: Code to report the module's status.
//    // For example, print the loop_counter.
//    return true;
//}
//
//bool ControllerModuleTemplate::enable() {
//    // TODO: Code to enable the module's operation.
//    return true;
//}
//
//bool ControllerModuleTemplate::disable() {
//    // TODO: Code to disable the module's operation.
//    return true;
//}
//
//// --- Protected Getters for Module ---
//
//const CommandParser::Command* ControllerModuleTemplate::get_commands() const {
//    return ControllerModuleTemplate::COMMANDS;
//}
//
//size_t ControllerModuleTemplate::get_commands_num() const {
//    return ControllerModuleTemplate::NUM_COMMANDS;
//}
//
//const String& ControllerModuleTemplate::get_mem_key() const {
//    return ControllerModuleTemplate::MODULE_MEM_KEY;
//}