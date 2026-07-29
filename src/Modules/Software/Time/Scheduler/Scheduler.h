// src/Modules/Software/Scheduler/Scheduler.h
#pragma once

#include "../../../Module/Module.h"

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    explicit Scheduler(ModuleController& controller);

    void                        begin_routines_init(const ModuleConfig& cfg) override; //only happens on first boot
    void                        begin_routines_common(const ModuleConfig& cfg) override; // happen on every boot but the first one

    void                        loop() override;
    void                        reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;
    std::string                 status(bool verbose=false) const override;

    void                        add                         (sid, start_time,...);

    void                        remove                      (uint32_t button_id);

    void                        load_from_nvs               ();
    void                        save_to_nvs                 ();


private:
    struct ScheduleBlock {
        uint8_t id;
        uint16_t start_time; //minutes from midnight
        uint16_t end_time;
        uint8_t day;
        std::string displayed_color; // 6 chars (RRGGBB)
        std::vector<std::string> commands;
    };

    std::vector<ScheduleBlock> schedules;

    void load_from_nvs();
    void save_to_nvs();

    // CLI Callbacks
    void cli_add(std::string_view args);
    void cli_remove(std::string_view args);
};