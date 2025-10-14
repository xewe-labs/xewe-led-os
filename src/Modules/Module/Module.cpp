// src/Modules/Module/Module.cpp
#include "Module.h"
#include "../../SystemController/SystemController.h"

using namespace xewe::str;

void Module::begin (const ModuleConfig& cfg) {
    DBG_PRINTF(Module, "'%s'->begin(): Called.\n", module_name.c_str());
    controller.serial_port.print_spacer();
    controller.serial_port.print_centered(capitalize(module_name) + " Setup", 50);
    controller.serial_port.print_spacer();

    // always enabled on first startup, and if module configured to enabled
    enabled = !init_setup_complete() || controller.nvs.read_bool(nvs_key, "is_en");
    if (is_disabled(true)) return;

    begin_routines_required(cfg);

    if (!init_setup_complete()) {
        controller.nvs.write_bool(nvs_key, "is_en", true);
        // inti setup req check
        if (!requires_init_setup) {
            begin_routines_common(cfg);
            controller.nvs.write_bool(nvs_key, "isc", true);
            return;
        }
        DBG_PRINTLN(Module, "begin(): Module requires initial setup and it is not yet complete. Calling init_setup().");
        // user enabled module
        if (can_be_disabled) {
            enabled = controller.serial_port.prompt_user_yn(std::string("Would you like to enable ") + capitalize(module_name) + " module?\n" + module_description);
        }
        if (!enabled) {
            controller.nvs.write_bool(nvs_key, "is_en", false);
            controller.nvs.write_bool(nvs_key, "isc", true);
            return;
        }

        begin_routines_init(cfg);
        controller.nvs.write_bool(nvs_key, "isc", true);
    }
    else {
        begin_routines_regular(cfg);
    }
    begin_routines_common(cfg);
}

void Module::begin_routines_required(const ModuleConfig&) {}
void Module::begin_routines_init(const ModuleConfig&) {}
void Module::begin_routines_regular(const ModuleConfig&) {}
void Module::begin_routines_common(const ModuleConfig&) {}

void Module::loop() {}
void Module::reset(const bool verbose) {}

// returns success of the operation
bool Module::enable(bool verbose) {
    DBG_PRINTF(Module, "'%s'->enable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_enabled()){
        DBG_PRINTLN(Module, "enable(): Module is already enabled.");
        Serial.printf("%s module already enabled\n", module_name.c_str());
        return false;
    }
    enabled = true;
    DBG_PRINTLN(Module, "enable(): Writing 'is_en'=true to NVS.");
    controller.nvs.write_bool(nvs_key, "is_en", true);
    if (verbose) Serial.printf("%s module enabled. Restarting...\n", module_name.c_str());
    ESP.restart();
    return true;
}

// returns success of the operation
bool Module::disable(bool verbose) {
    DBG_PRINTF(Module, "'%s'->disable(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (is_disabled()){
        DBG_PRINTLN(Module, "disable(): Module is already disabled.");
        if (verbose) Serial.printf("%s module already disabled\n", module_name.c_str());
        return false;
    }
    if (!can_be_disabled) {
        DBG_PRINTLN(Module, "disable(): Module cannot be disabled.");
        if (verbose) Serial.printf("%s module can't be disabled\n", module_name.c_str());
        return false;
    }
    if (verbose) Serial.printf("%s module disabled. Restarting...\n", module_name.c_str());
    DBG_PRINTLN(Module, "disable(): Writing 'is_en'=false to NVS.");
    enabled = false;
    controller.nvs.write_bool(nvs_key, "is_en", false);
    ESP.restart();
    return true;
}

std::string Module::status(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->status(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    std::string status_str = (module_name + " module " + (controller.nvs.read_bool(nvs_key, "is_en") ? "enabled" : "disabled"));
    DBG_PRINTF(Module, "status(): Generated status string: '%s'.\n", status_str.c_str());
    if (verbose) Serial.printf("%s\n", status_str.c_str());
    return status_str;
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_enabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        DBG_PRINTF(Module, "is_enabled(): Module can be disabled, read NVS 'is_en' flag as %s.\n", enabled ? "true" : "false");
        if (verbose && enabled) Serial.printf("%s module enabled\n", module_name.c_str());
        return enabled;
    }
    DBG_PRINTLN(Module, "is_enabled(): Module cannot be disabled, returning true by default.");
    return true;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    DBG_PRINTF(Module, "'%s'->is_disabled(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    if (can_be_disabled) {
        if (verbose && !enabled) Serial.printf("%s module disabled; use $%s enable\n", module_name.c_str(), lower(module_name).c_str());
        return !enabled;
    }
    DBG_PRINTLN(Module, "is_disabled(): Module cannot be disabled, returning false by default.");
    return false;
}

bool Module::init_setup_complete (bool verbose) const {
    DBG_PRINTF(Module, "'%s'->init_setup_complete(verbose=%s): Called.\n", module_name.c_str(), verbose ? "true" : "false");
    bool isc_flag = controller.nvs.read_bool(nvs_key, "isc");
    bool result = !requires_init_setup || isc_flag;
    DBG_PRINTF(Module, "init_setup_complete(): requires_init_setup=%s, nvs 'isc' flag=%s. Final result=%s\n",
        requires_init_setup ? "true" : "false",
        isc_flag ? "true" : "false",
        result ? "true" : "false"
    );
    return result;
}

CommandsGroup Module::get_commands_group() {
    DBG_PRINTF(Module, "'%s'->get_commands_group(): Called.\n", module_name.c_str());
    commands_group.name     = module_name;
    commands_group.group     = lower(module_name);
    commands_group.commands = std::span<const Command>(
        commands_storage.data(),
        commands_storage.size()
    );
    DBG_PRINTF(Module, "get_commands_group(): Returning command group '%s' with %zu commands.\n", commands_group.name.c_str(), commands_storage.size());
    return commands_group;
}

void Module::run_with_dots(const std::function<void()>& work, uint32_t duration_ms, uint32_t dot_interval_ms) {
  if (dot_interval_ms == 0) dot_interval_ms = 1;

  const uint32_t start = millis();
  uint32_t next = start;  // first dot at t=0

  while ((uint32_t)(millis() - start) < duration_ms) {
    work();  // run the target function

    const uint32_t now = millis();
    if ((int32_t)(now - next) >= 0) {
      controller.serial_port.print(std::string_view{"."});

      // If we're late by multiple intervals, skip ahead (prevents dot bursts)
      const uint32_t late = now - next;
      const uint32_t intervals = 1u + (late / dot_interval_ms);
      next += intervals * dot_interval_ms;
    }
  }
  controller.serial_port.print(std::string_view{"\n"});
}
