#include "SystemController.h"
#include <cstring>

SystemController::SystemController()
  : Module(*this,
           /* module_name       */ "system_controller",
           /* initially_enabled */ true,
           /* can_be_disabled   */ false,
           /* nvs_key_param     */ "sys_ctrl")
  , cmd_parser(*this)
{
    modules[0] = &cmd_parser;
}

void SystemController::begin(void* context) {
    for (auto m : modules) {
        m->begin(context);
    }
}

void SystemController::loop(void* context) {
    for (auto m : modules) {
        m->loop(context);
    }
}

void SystemController::enable() {
    for (auto m : modules) {
        m->enable();
    }
}

void SystemController::disable() {
    for (auto m : modules) {
        m->disable();
    }
}

void SystemController::reset() {
    for (auto m : modules) {
        m->reset();
    }
}

const char* SystemController::status() {
    return enabled ? "enabled" : "disabled";
}

void SystemController::enable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmd_parser.enable();
    }
}

void SystemController::disable_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmd_parser.disable();
    }
}

void SystemController::reset_module(const char* module_name) {
    if (std::strcmp(module_name, "command_parser") == 0) {
        cmd_parser.reset();
    }
}

const char* SystemController::module_status(const char* module_name) const {
    if (std::strcmp(module_name, "command_parser") == 0) {
        return cmd_parser.status();
    }
    return nullptr;
}
