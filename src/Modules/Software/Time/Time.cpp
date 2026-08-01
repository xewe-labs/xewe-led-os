#include "Time.h"
#include "../../Module/ModuleController.h"


Time::Time(ModuleController& controller)
    : Module(controller, "time", "Time", "Handles NTP and Timezone", true, true, true)
{
    commands_storage.push_back(Command{
        "set_zone",
        "Set timezone offset (e.g. GMT-08:00)",
        "$time set_zone GMT-08:00",
        1,
        [this](std::span<const std::string> args){ cli_set_timezone(args); }
    });
    commands_storage.push_back(Command{
        "fetch",
        "Get current time from the web",
        "$time fetch",
        0,
        [this](std::span<const std::string> args){ cli_fetch(args); }
    });
}


void Time::fetch_tz_task(void* pvParameters) {
    TzArg* arg = static_cast<TzArg*>(pvParameters);
    TzRace* ctx = arg->ctx;

    esp_http_client_config_t config = {};
    config.url = arg->url;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "User-Agent", "ESP32-Time-Module/1.0");
    esp_http_client_set_header(client, "Accept", "application/json");

    if (esp_http_client_open(client, 0) == ESP_OK) {
        esp_http_client_fetch_headers(client);

        char buffer[1024] = {0};
        int total_read = 0;
        int read_len = 0;

        while (!ctx->abort.load() &&
               (read_len = esp_http_client_read(client, buffer + total_read, sizeof(buffer) - total_read - 1)) > 0) {
            total_read += read_len;
            if (total_read >= sizeof(buffer) - 1) break;
        }

        if (!ctx->abort.load() && total_read > 0) {
            buffer[total_read] = '\0';
            std::string resp(buffer);

            size_t pos = resp.find(arg->key);
            if (pos != std::string::npos) {
                size_t start_quote = resp.find('"', pos + std::strlen(arg->key));
                if (start_quote != std::string::npos && start_quote + 7 <= resp.length()) {
                    std::string offset_str = resp.substr(start_quote + 1, 6);
                    std::string normalized_gmt;

                    if (xewe::str::parse_gmt_offset("GMT" + offset_str, normalized_gmt) &&
                        ctx->claimed.exchange(1) == 0) {
                        strncpy(ctx->result, normalized_gmt.c_str(), sizeof(ctx->result) - 1);
                        xSemaphoreGive(ctx->winner);
                    }
                }
            }
        }
    }

    if (client) esp_http_client_cleanup(client);
    xSemaphoreGive(ctx->done);   // last touch of ctx
    delete arg;
    vTaskDelete(NULL);
}

void Time::begin_routines_required(const ModuleConfig& cfg) {
    get_time_from_web_init();
}

void Time::begin_routines_init(const ModuleConfig& cfg) {
    controller.serial_port.print("Detecting Timezone...", "");

    const char* TZ_ENDPOINTS[] = {
        "http://ipwho.is/?fields=success,timezone.offset,timezone.utc",
        "http://ipwhois.app/json/",
        "http://api.ip2location.io/"
    };

    const char* TZ_SEARCH_KEYS[] = {
        "\"utc\"",
        "\"timezone_gmt\"",
        "\"time_zone\""
    };

    TzRace ctx;
    ctx.winner = xSemaphoreCreateBinary();
    ctx.done   = xSemaphoreCreateCounting(3, 0);

    for (int i = 0; i < 3; i++) {
        xTaskCreate(fetch_tz_task, "fetch_tz_task", 4096,
                    new TzArg{TZ_ENDPOINTS[i], TZ_SEARCH_KEYS[i], &ctx}, 5, NULL);
    }

    bool tz_found = false;
    for (int i = 0; i < 30 && !tz_found; i++) {
        tz_found = xSemaphoreTake(ctx.winner, pdMS_TO_TICKS(200)) == pdTRUE;
        if (!tz_found) controller.serial_port.print(".", "");
    }

    ctx.abort.store(true);
    for (int i = 0; i < 3; i++) xSemaphoreTake(ctx.done, portMAX_DELAY);  // join workers
    vSemaphoreDelete(ctx.winner);
    vSemaphoreDelete(ctx.done);
    controller.serial_port.print();

    if (tz_found) {
        get_time_from_web_wait(true);
        std::string gmt_str(ctx.result);
        apply_timezone(gmt_str);
        tm ct = get_current_time();

        char prompt[128];
        snprintf(prompt, sizeof(prompt), "Is your time: %04d-%02d-%02d %02d:%02d:%02d (%s)?",
                 ct.tm_year + 1900, ct.tm_mon + 1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, gmt_str.c_str());

        if (controller.serial_port.get_yn(prompt)) {
            controller.nvs.write<std::string>(id, "tz_gmt_str", gmt_str);
            controller.serial_port.print("Timezone set");
            return;
        }
    } else {
        controller.serial_port.print("Unable to reach timezone server.\nCheck your internet connection.\n");
    }

    while (true) {
        std::string normalized_gmt;
        std::string tz_input = controller.serial_port.get_string("Enter your timezone offset (e.g. GMT-08:00)\nFor support visit:\nhttps://webbrowsertools.com/timezone/");

        if (xewe::str::parse_gmt_offset(tz_input, normalized_gmt)) {
            apply_timezone(normalized_gmt);
            controller.nvs.write<std::string>(id, "tz_gmt_str", normalized_gmt);
            controller.serial_port.printf("Timezone set to %s\n", normalized_gmt.c_str());
            return;
        }
    }
}

void Time::begin_routines_regular(const ModuleConfig& cfg) {
    apply_timezone(controller.nvs.read<std::string>(id, "tz_gmt_str", "GMT+00:00"));
    if (get_time_from_web_wait(true)) {
        print_current_time();
    } else {
        controller.serial_port.print("Unable to reach time server.\nCheck your internet connection.\nTo retry: $time fetch");
        time_set = false;
    }
}


void Time::get_time_from_web_init(const bool verbose) {
    esp_netif_sntp_deinit();

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(
        3, ESP_SNTP_SERVER_LIST("pool.ntp.org", "time.google.com", "time.cloudflare.com")
    );
    esp_netif_sntp_init(&sntp_cfg);
}

bool Time::get_time_from_web_wait(const bool verbose) {
    if (verbose) controller.serial_port.print("Syncing time from server...", "");

    uint8_t retries = 0;
    esp_err_t sync_err = ESP_FAIL;
    while ((sync_err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(200))) != ESP_OK && retries < 50) {
        if (verbose) controller.serial_port.print(".", "");
        vTaskDelay(pdMS_TO_TICKS(200));
        retries++;
    }
    controller.serial_port.print();
    time_set = (sync_err == ESP_OK);
    return time_set;
}

void Time::reset(bool verbose, bool do_restart, bool keep_enabled) {
    controller.nvs.remove(id, "tz_gmt_str");
    Module::reset(verbose, do_restart, keep_enabled);
}

std::string Time::status(bool verbose) const {
    if (is_disabled()) return Module::status();
    std::string current_time_str = get_current_time_str();
    if (verbose) controller.serial_port.print(current_time_str);
    return current_time_str;
}

void Time::apply_timezone(std::string_view gmt_offset_str) {
    active_tz_string = std::string(gmt_offset_str);

    std::string posix_tz = std::string(gmt_offset_str);
    if (posix_tz.length() >= 4) {
        posix_tz[3] = (posix_tz[3] == '-' ? '+' : '-');
    }

    setenv("TZ", posix_tz.c_str(), 1);
    tzset();
}

tm Time::get_current_time() const {
    const time_t now = time(nullptr);
    tm tm_now{};
    localtime_r(&now, &tm_now);
    return tm_now;
}

std::string Time::get_current_time_str() const {
    if (is_disabled()) return "Time module disabled";
    if (!time_set) return "Time is not set";

    tm current_time = get_current_time();
    char time_str[64];
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
             current_time.tm_year + 1900, current_time.tm_mon + 1, current_time.tm_mday,
             current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
    return std::string(time_str);
}

void Time::cli_set_timezone(std::span<const std::string> args) {
    std::string normalized_gmt;
    if (xewe::str::parse_gmt_offset(args[0], normalized_gmt)) {
        apply_timezone(normalized_gmt);
        controller.nvs.write<std::string>(id, "tz_gmt_str", normalized_gmt);
        controller.serial_port.print("Timezone updated.");
    } else {
        controller.serial_port.print("Invalid format. Use GMT±HH:MM (e.g., GMT-08:00).");
    }
}

void Time::cli_fetch(std::span<const std::string> args) {
    get_time_from_web_init(true);
    if (get_time_from_web_wait(true)) {
        print_current_time();
    } else {
        controller.serial_port.print("Unable to reach time server.\nCheck your internet connection.");
        time_set = false;
    }
}

void Time::print_current_time() {
    if (is_disabled()) controller.serial_port.print("Time module disabled");
    if (!time_set) controller.serial_port.print("Time is not set");

    controller.serial_port.print("Current time: " + get_current_time_str());
}