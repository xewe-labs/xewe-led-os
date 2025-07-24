#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include "Modules/Module.h"
#include "Modules/Software/CommandParser/CommandParser.h"

class SystemController : public Module<void> {
public:
    SystemController();

    void begin(void* context = nullptr) override;
    void loop(void* context = nullptr) override;
    void enable() override;
    void disable() override;
    void reset() override;
    const char* status() override;

    void enable_module(const char* module_name);
    void disable_module(const char* module_name);
    void reset_module(const char* module_name);
    const char* module_status(const char* module_name) const;

private:
    CommandParser    cmd_parser;
    Module<void>*    modules[1];
};

#endif // SYSTEM_CONTROLLER_H
