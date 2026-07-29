#pragma once

#include <optional>
#include <ctime>
#include <string>
#include <string_view>
#include <span>
#include <Arduino.h>
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "../../Module/Module.h"

struct TimeConfig : public ModuleConfig {};

class Time : public Module {
public:
    explicit Time(ModuleController& controller);

    void begin_routines_init(const ModuleConfig& cfg) override;
    void begin_routines_regular(const ModuleConfig& cfg) override;

    void reset(bool verbose=false, bool do_restart=true, bool keep_enabled=true) override;

    std::string status(bool verbose=false) const override;

    tm get_current_time() const;
    void print_current_time();

private:
    int16_t active_timezone_bias_min{0};

    bool get_time_from_web(bool verbose=true);
    void apply_timezone(int16_t bias_minutes);

    void cli_set_timezone(std::span<const std::string> args);

    struct TzTaskParam {
        const char* url;
        const char* search_key;
        QueueHandle_t result_queue;
    };

    static void http_tz_task(void* pvParameters);
};