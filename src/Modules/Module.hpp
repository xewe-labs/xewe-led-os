// Module.h
#ifndef MODULE_H
#define MODULE_H

#include "../SystemController.h"            // core system services interface
#include "Software/CommandParser/CommandParser.h" // command parser types

class Module {
public:
    /**
     * @param controller_ref     Reference to the shared SystemController
     * @param initially_enabled  Whether this module starts enabled
     * @param can_be_disabled    Whether disable()/enable() actually have effect
     * @param nvs_key_param      Key under which to store persistent state in NVS
     */
    Module(SystemController& controller_ref,
           bool initially_enabled,
           bool can_be_disabled,
           const char* nvs_key_param)
      : controller(controller_ref)
      , enabled(initially_enabled)
      , can_be_disabled(can_be_disabled)
      , nvs_key(nvs_key_param)
    {}

    virtual                     ~Module() noexcept = default;

    /// Called once from your setup(), optional device_name
    virtual void                begin(void* context = nullptr) = 0;

    /// Called continually from your loop()
    virtual void                loop() = 0;

    /// Turn this module on (if can_be_disabled)
    virtual void                enable() = 0;

    /// Turn this module off (if can_be_disabled)
    virtual void                disable() = 0;

    /// Reset internal state (e.g. clear NVS flag, restart sensors)
    virtual void                reset() = 0;

    /// Return a short status C-string (avoid heap churn)
    virtual const char*             status() = 0;

protected:
    SystemController&               controller;       ///< core system services
    const char*                     nvs_key;          ///< NVS key in flash

    bool                            enabled;          ///< current enabled state
    bool                            can_be_disabled;  ///< whether enable/disable work

    static constexpr uint8_t        cmd_count = 0;      ///< override in derived if commands needed
    CommandParser::Command          commands[cmd_count];
    CommandParser::CommandGroup     commands_group;
};

#endif // MODULE_H
