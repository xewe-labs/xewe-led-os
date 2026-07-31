// src/Modules/Software/Time/Time.h
#pragma once

#include <optional>
#include <ctime>
#include <string>
#include <string_view>
#include <span>
#include <Arduino.h>
#include <atomic>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "../../Module/Module.h"

struct TimeConfig : public ModuleConfig {};

class Time : public Module {
public:
    explicit Time(ModuleController& controller);

    void begin_routines_required(const ModuleConfig& cfg) override;
    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_regular(const ModuleConfig& cfg) override;

    void reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;
    std::string status(bool verbose=false) const override;

    tm get_current_time() const;
    std::string get_current_time_str() const;
    void print_current_time();

private:
    bool    time_set {false};
    std::string active_tz_string{"GMT+00:00"};

    void get_time_from_web_init(const bool verbose=true);
    bool get_time_from_web_wait(const bool verbose=true);
    void apply_timezone(std::string_view gmt_offset_str);

    void cli_set_timezone(std::span<const std::string> args);
    void cli_fetch(std::span<const std::string> args);

    struct TzRace {
        std::atomic<bool> abort{false};
        std::atomic<int>  claimed{0};   // CAS 0->1 selects the single winner
        char result[16]{};
        SemaphoreHandle_t winner;       // binary, given once by the winner
        SemaphoreHandle_t done;         // counting(3,0), given once per worker on exit
    };
    struct TzArg { const char* url; const char* key; TzRace* ctx; };

    static void fetch_tz_task(void* pvParameters);
};