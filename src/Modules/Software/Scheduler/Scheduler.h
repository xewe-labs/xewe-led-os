// src/Modules/Software/Scheduler/Scheduler.h
#pragma once

#include "../../Module/Module.h"
#include <vector>
#include <string>
#include <string_view>

struct SchedulerConfig : public ModuleConfig {};

class Scheduler : public Module {
public:
    explicit Scheduler(SystemController& controller);

    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_regular(const ModuleConfig& cfg) override;

    void loop() override;
    void reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;
    string status(bool verbose=false) const override;

    // Frontend API method
    std::string get_all_json() const;

private:
    struct ScheduleBlock {
        std::string id;
        uint8_t day{0};
        uint16_t start_minute{0};
        uint16_t end_minute{0};
        std::string color;
        std::vector<std::string> commands;
        std::string config_str;
        int32_t last_executed_daystamp{-1};
    };

    std::vector<ScheduleBlock> schedules;

    bool loaded_from_nvs{false};
    bool timezone_ready{false};
    bool time_ready{false};
    int32_t active_timezone_bias_min{0};

    // Core functionality
    bool add_schedule(const std::string& config);
    void remove_schedule(const std::string& id);
    void apply_timezone(int32_t bias_minutes);

    // NVS Management
    void load_from_nvs();
    void nvs_append_config(const std::string& cfg);
    void nvs_rewrite_all_configs();
    void nvs_clear_all();

    // CLI Callbacks
    void cli_add(std::string_view args);
    void cli_remove(std::string_view args);
    void cli_timezone(std::string_view args);
    void cli_print_schedules(std::string_view args);
};