// src/Modules/Software/Scheduler/Scheduler.h
#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <sstream> // Required locally for the extraction stream in `add()`

#include "../../Module/Module.h"

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    explicit Scheduler(SystemController& controller);

    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_common(const ModuleConfig& cfg) override;

    void loop() override;
    void reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;
    std::string status(bool verbose=false) const override;

    // Requested APIs
    bool add(const std::string& config);
    void remove(uint8_t id);
    std::string get_all_json() const;

private:
    struct ScheduleBlock {
        uint8_t id;
        uint16_t start_time;
        uint16_t end_time;
        uint8_t day;
        std::string displayed_color; // 6 chars (RRGGBB)
        std::vector<std::string> commands;
    };

    std::vector<ScheduleBlock> schedules;
    bool loaded_from_nvs{false};
    uint16_t last_executed_minute{60000}; // Tracks the last minute we fired schedules

    // Robust NVS Management
    void load_from_nvs();
    void nvs_save_active_ids();
    void nvs_save_config(uint8_t id, const std::string& cfg);
    void nvs_delete_config(uint8_t id);
    void nvs_clear_all();

    // CLI Callbacks
    void cli_add(std::string_view args);
    void cli_remove(std::string_view args);
    void cli_print_schedules(std::string_view args);
};