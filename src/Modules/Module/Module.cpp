// SPDX-FileCopyrightText: 2026 Maxim Dokukin (maxdokukin.com)
// SPDX-License-Identifier: GPL-3.0-only
// src/Modules/Module/Module.cpp

#include "Module.h"
#include "ModuleController.h"


void Module::begin (const ModuleConfig& cfg) {
    bool first_boot = !controller.nvs.read<bool>(id, "not_first_boot");
    enabled = first_boot || controller.nvs.read<bool>(id, "is_enabled");

    if (can_be_disabled || requires_init_setup) {
         controller.serial_port.print_header(name + " Setup");
    }

    if (!requirements_enabled(true)) {
        enabled = false;
        controller.nvs.write<bool>(id, "is_enabled", false);
        controller.nvs.write<bool>(id, "not_first_boot", true);
        return;
    } else {
        // if the module was disabled due to inactive requirements, re-enable
        if (!can_be_disabled) {
            enabled = true;
            controller.nvs.write<bool>(id, "is_enabled", true);
        }
    }

    if (is_disabled(true)) return;

    if (first_boot) {
        if (can_be_disabled) {
            controller.serial_port.print_header(std::string("Would you like to enable ") + name + " module?\n\n" + description);
            enabled = controller.serial_port.get_yn();

            if (!enabled) {
                controller.nvs.write<bool>(id, "is_enabled", false);
                controller.nvs.write<bool>(id, "not_first_boot", true);
                return;
            }
        }
        controller.nvs.write<bool>(id, "is_enabled", true);
        controller.nvs.write<bool>(id, "not_first_boot", true);
    }

    begin_routines_required(cfg);

    if (!init_setup_complete()) {
        begin_routines_init(cfg);
        if (enabled) { // could have been disabled during begin_routines_init()
            controller.nvs.write<bool>(id, "init_complete", true);
        }
    } else {
        begin_routines_regular(cfg);
    }

    begin_routines_common(cfg);
}

void Module::begin_routines_required(const ModuleConfig&) {}
void Module::begin_routines_init(const ModuleConfig&) {}
void Module::begin_routines_regular(const ModuleConfig&) {}
void Module::begin_routines_common(const ModuleConfig&) {}

void Module::add_requirement(Module& other) {
    if (&other == this) return;
    required_modules.push_back(&other);
    other.dependent_modules.push_back(this);
}

void Module::loop() {}

void Module::enable(const bool verbose, const bool do_restart) {
    if (is_enabled()){
        Serial.printf("%s module already enabled\n", name.c_str());
        return;
    }

    if (!requirements_enabled(true)) return;

    enabled = true;
    controller.nvs.write<bool>(id, "is_enabled", true);

    if (verbose) Serial.printf("%s module enabled. Restarting...\n\n\n", name.c_str());

    ESP.restart();
}

void Module::disable(const bool verbose, const bool do_restart) {
    if (is_disabled()){
        if (verbose) Serial.printf("%s module already disabled\n", name.c_str());
        return;
    }
    if (!can_be_disabled) {
        if (verbose) Serial.printf("%s module can't be disabled\n", name.c_str());
        return;
    }

    bool disable_confirmed = true;

    if (verbose) {
        std::string msg = "[WARNING]\nDisabling " + name + "\nWill reset it";
        if (!dependent_modules.empty()) {
            msg += ", and all dependents: \\sep";
            for (auto* m : dependent_modules) {
                msg += m->name + "\n";
            }
            if (!msg.empty() && msg.back() == '\n')
               msg.pop_back();
        }
        controller.serial_port.print_header(msg);
        disable_confirmed = controller.serial_port.get_yn("OK?");
    }

    if (!disable_confirmed) {
        controller.serial_port.print("Aborted");
        return;
    }

    if (!dependent_modules.empty()) {
        for (auto* m : dependent_modules) {
            if (verbose) Serial.printf("%s module reset and disabled\n", m->name.c_str());
            m->disable(false, false); // cascade disable with no verbose, and dont reboot
        }
    }
    if (verbose) {
        Serial.printf("%s module disabled\n", name.c_str());
    }

    reset(verbose, do_restart, false);
    return;
}

void Module::reset(const bool verbose, const bool do_restart, const bool keep_enabled) {
    controller.nvs.reset_ns(id);
    controller.nvs.write<bool>(id, "not_first_boot", true);

    enabled = (!can_be_disabled || keep_enabled) && requirements_enabled();

    if (enabled) { // re-enable the module
        controller.nvs.write<bool>(id, "is_enabled", true);
    }

    if (verbose) Serial.printf("%s module reset\n", name.c_str());

    if (do_restart) {
        if (verbose) Serial.printf("Restarting...\n\n\n");
        ESP.restart();
    }
}

std::string Module::status(bool verbose) const {
    std::string status_str = (name + " module " + (controller.nvs.read<bool>(id, "is_enabled") ? "enabled" : "disabled"));
    if (verbose) Serial.printf("%s\n", status_str.c_str());
    return status_str;
}

// only print the debug msg if true
bool Module::is_enabled(bool verbose) const {
    if (verbose && enabled) Serial.printf("%s module enabled\n", name.c_str());
    return enabled;
}

// only print the debug msg if true
bool Module::is_disabled(bool verbose) const {
    if (verbose && !enabled) {
        // case 1: requirements are not enabled
        if (!requirements_enabled()) {
            Serial.printf("%s module disabled\n", name.c_str());
            requirements_enabled(true); // this will print list of requirements
        // case 2: disabled by user
        } else {
            Serial.printf("%s module disabled; to enable:\n$%s enable\n", name.c_str(), id.c_str());
        }
    }
    return !enabled;
}

bool Module::init_setup_complete (bool verbose) const {
    return !requires_init_setup || controller.nvs.read<bool>(id, "init_complete");
}

void Module::register_generic_commands() {
    commands_storage.push_back(Command{
        "status",
        "Get module status",
        std::string("$") + id + " status",
        0,
        [this](std::span<const std::string>) {
            status(true);
        }
    });

    commands_storage.push_back(Command{
        "reset",
        "Reset the module",
        std::string("$") + id + " reset",
        0,
        [this](std::span<const std::string>) {
            reset(true, true);
        }
    });

    if (can_be_disabled) {
        commands_storage.push_back(Command{
            "enable",
            "Enable this module",
            std::string("$") + id + " enable",
            0,
            [this](std::span<const std::string>) {
                enable(true, true);
            }
        });

        commands_storage.push_back(Command{
            "disable",
            "Disable this module",
            std::string("$") + id + " disable",
            0,
            [this](std::span<const std::string>) {
                disable(true, true);
            }
        });
    }
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

bool Module::requirements_enabled(bool verbose) const {
    bool all_enabled = true;
    bool printed_header = false;

    for (auto* r : required_modules) {
        if (r->is_disabled()) {
            if (!verbose) return false;

            all_enabled = false;

            if (!printed_header) {
                printed_header = true;
                Serial.printf("%s module requires:\n", name.c_str());
            }
            Serial.printf("%s, use: $%s enable\n", r->name.c_str(), r->id.c_str());
        }
    }
    return all_enabled;
}

std::span<const Command> Module::get_commands () const {
    return std::span<const Command>(commands_storage.data(), commands_storage.size());
}
