/*********************************************************************************
 *  SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 *  Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 *  See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 *  Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 *  https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/
// src/Interfaces/Hardware/LedStrip/ModeController/ModeController.cpp

#include "ModeController.h"

ModeController::ModeController(CRGB* output_buffer,
                               uint16_t num_leds,
                               uint16_t transition_delay_ms,
                               Nvs& nvs,
                               std::string_view nvs_namespace)
    : num_leds(num_leds),
      output_buffer(output_buffer),
      buffer_old_static_flag(false),
      nvs(nvs),
      nvs_namespace(nvs_namespace)
{
    DBG_PRINTLN(ModeController, "-> ModeController::ModeController()");
    DBG_PRINTF(ModeController, "Init config - Num LEDs: %u, Transition Delay: %u ms\n", num_leds, transition_delay_ms);

    fill_solid(buffer_current.data(), LED_STRIP_NUM_LEDS_MAX, CRGB::Black);
    fill_solid(buffer_old.data(), LED_STRIP_NUM_LEDS_MAX, CRGB::Black);

    transition_timer = std::make_unique<AsyncTimer<uint8_t>>(transition_delay_ms, 0, 255);

    // On boot, restore mode 0 params from NVS if present.
    set_mode(0, {});

    DBG_PRINTLN(ModeController, "<- ModeController::ModeController()");
}

void ModeController::loop() {
    if (transition_timer->is_active()) {
        update_interpolate_buffers(output_buffer);
        if (transition_timer->is_done()) {
            DBG_PRINTLN(ModeController, "[ModeController] Transition finished. Terminating timer.");
            transition_timer->terminate();
            buffer_old_static_flag = false;
        }
        return;
    }

    current_mode->loop(output_buffer, num_leds);
}

void ModeController::set_mode(const uint8_t mode_id, const std::map<std::string, uint16_t>& params) {
    DBG_PRINTF(ModeController, "-> ModeController::set_mode(mode_id: %u)\n", mode_id);

    auto& registry = ModeRegistry::get_registry();
    const uint8_t effective_mode_id = registry.count(mode_id) ? mode_id : 0;
    ModeFactory factory = registry.at(effective_mode_id);

    // Resolution order:
    // 1. mode defaults
    // 2. stored NVS values
    // 3. caller-provided overrides
    std::map<std::string, uint16_t> resolved_params = get_default_params_for_mode(effective_mode_id);

    const auto nvs_params = load_mode_params_from_nvs(effective_mode_id);
    for (const auto& [key, value] : nvs_params) {
        resolved_params[key] = value;
    }

    for (const auto& [key, value] : params) {
        resolved_params[key] = value;
    }

    if (transition_timer->is_active()) {
        update_interpolate_buffers(buffer_old.data());
        buffer_old_static_flag = true;
    }

    old_mode = std::move(current_mode);
    current_mode = factory(resolved_params);

    // Persist sanitized/live values after mode creation.
    persist_mode_params_to_nvs(current_mode->get_id());

    transition_timer->reset();
    transition_timer->initiate();

    DBG_PRINTLN(ModeController, "<- ModeController::set_mode()");
}

bool ModeController::set_mode_param(std::string_view key, uint16_t value) {
    DBG_PRINTF(ModeController, "-> ModeController::set_mode_param(key: %.*s, value: %u)\n",
               (int)key.length(), key.data(), value);

    auto params_map = get_params_as_map();
    auto it = params_map.find(std::string(key));

    if (it == params_map.end()) {
        DBG_PRINTLN(ModeController, "<- ModeController::set_mode_param() [Failed: Key not found]");
        return false;
    }

    it->second = normalize_mode_param_value(key, value);
    set_mode(get_current_mode_id(), params_map);

    DBG_PRINTLN(ModeController, "<- ModeController::set_mode_param() [Success]");
    return true;
}

void ModeController::set_rgb(const std::array<uint8_t, 3> new_rgb) {
    DBG_PRINTF(ModeController, "-> ModeController::set_rgb(%u, %u, %u)\n", new_rgb[0], new_rgb[1], new_rgb[2]);

    auto params_map = get_params_as_map();

    if (params_map.count("hue") || params_map.count("sat")) {
        const std::array<uint8_t, 3> new_hsv = rgb_to_hsv(new_rgb);
        if (params_map.count("hue")) params_map["hue"] = new_hsv[0];
//        if (params_map.count("hue_a")) params_map["hue_a"] = new_hsv[0];
        if (params_map.count("sat")) params_map["sat"] = new_hsv[1];
    }

    if (params_map.count("r")) params_map["r"] = new_rgb[0];
    if (params_map.count("g")) params_map["g"] = new_rgb[1];
    if (params_map.count("b")) params_map["b"] = new_rgb[2];

    set_mode(get_current_mode_id(), params_map);

    DBG_PRINTLN(ModeController, "<- ModeController::set_rgb()");
}

void ModeController::set_hsv(const std::array<uint8_t, 3> new_hsv) {
    DBG_PRINTF(ModeController, "-> ModeController::set_hsv(%u, %u, %u)\n", new_hsv[0], new_hsv[1], new_hsv[2]);

    auto params_map = get_params_as_map();

    if (params_map.count("r") || params_map.count("g") || params_map.count("b")) {
        const std::array<uint8_t, 3> new_rgb = hsv_to_rgb(new_hsv);
        if (params_map.count("r")) params_map["r"] = new_rgb[0];
        if (params_map.count("g")) params_map["g"] = new_rgb[1];
        if (params_map.count("b")) params_map["b"] = new_rgb[2];
    }

    if (params_map.count("hue")) params_map["hue"] = new_hsv[0];
//    if (params_map.count("hue_a")) params_map["hue_a"] = new_hsv[0];
    if (params_map.count("sat")) params_map["sat"] = new_hsv[1];

    set_mode(get_current_mode_id(), params_map);

    DBG_PRINTLN(ModeController, "<- ModeController::set_hsv()");
}

bool ModeController::adj_mode_param(std::string_view key, int32_t value_delta) {
    DBG_PRINTF(ModeController, "-> ModeController::adj_mode_param(key: %.*s, delta: %ld)\n",
               (int)key.length(), key.data(), static_cast<long>(value_delta));

    for (const auto& param : current_mode->get_params()) {
        if (param.key == key) {
            const int32_t current_value = static_cast<int32_t>(current_mode->get_param(param.key));
            const uint16_t new_value = normalize_mode_param_value(key, current_value + value_delta);
            set_mode_param(key, new_value);

            DBG_PRINTLN(ModeController, "<- ModeController::adj_mode_param() [Success]");
            return true;
        }
    }

    DBG_PRINTLN(ModeController, "<- ModeController::adj_mode_param() [Failed: Key not found]");
}

void ModeController::reset_current_mode() {
    DBG_PRINTLN(ModeController, "-> ModeController::reset_current_mode()");

    if (!current_mode) {
        DBG_PRINTLN(ModeController, "<- ModeController::reset_current_mode() [No current mode]");
        return;
    }

    const uint8_t mode_id = get_current_mode_id();
    const auto default_params = get_default_params_for_mode(mode_id);

    // Rebuild the current mode using only its defaults.
    // set_mode() will also persist the sanitized/live values back to NVS.
    set_mode(mode_id, default_params);

    DBG_PRINTLN(ModeController, "<- ModeController::reset_current_mode()");
}

uint16_t ModeController::get_current_mode_param(std::string_view key) const {
    if (!current_mode) {
        return 0;
    }

    for (const auto& param : current_mode->get_params()) {
        if (param.key == key) {
            return current_mode->get_param(param.key);
        }
    }

    return 0;
}

const ModeConfig& ModeController::get_mode_config(uint8_t mode_id) const {
    static std::map<uint8_t, ModeConfig> config_cache;

    auto it = config_cache.find(mode_id);

    if (it == config_cache.end()) {
        auto& registry = ModeRegistry::get_registry();
        ModeFactory factory = registry.count(mode_id) ? registry.at(mode_id) : registry.at(0);

        auto temp_mode = factory({});
        it = config_cache.emplace(mode_id, temp_mode->get_config()).first;
    }

    return it->second;
}

std::string ModeController::get_all_modes_json() const {
    std::string json = "[";
    bool first_mode = true;

    const auto& registry = ModeRegistry::get_registry();

    for (const auto& [id, factory] : registry) {
        if (!first_mode) json += ",";
        first_mode = false;

        auto temp_mode = factory({});
        const auto& config = temp_mode->get_config();

        const bool is_active_mode = (id == get_current_mode_id());

        json += "{\"id\":" + std::to_string(config.id) +
                ",\"name\":\"" + config.name + "\"" +
                ",\"params\":[";

        bool first_param = true;
        for (const auto& p : config.params) {
            if (!first_param) json += ",";
            first_param = false;

            int output_value = p.default_value;
            if (is_active_mode) {
                output_value = current_mode->get_param(p.key);
            }

            json += "{\"key\":\"" + p.key + "\"" +
                    ",\"display_name\":\"" + p.display_name + "\"" +
                    ",\"min\":" + std::to_string(p.min_value) +
                    ",\"max\":" + std::to_string(p.max_value) +
                    ",\"step\":" + std::to_string(p.step_value) +
                    ",\"value\":" + std::to_string(output_value) +
                    ",\"type\":\"" + std::string(1, p.type) + "\"}";
        }

        json += "]}";
    }

    json += "]";
    return json;
}

void ModeController::update_interpolate_buffers(CRGB* output_buffer_ref) {
    if (!buffer_old_static_flag && old_mode) {
        old_mode->loop(buffer_old.data(), num_leds);
    }

    current_mode->loop(buffer_current.data(), num_leds);
    const uint8_t progress = transition_timer->get_current_value();

    for (uint16_t i = 0; i < num_leds; i++) {
        output_buffer_ref[i] = blend(buffer_old[i], buffer_current[i], progress);
    }
}

std::map<std::string, uint16_t> ModeController::get_params_as_map() const {
    std::map<std::string, uint16_t> map;

    if (!current_mode) {
        return map;
    }

    for (const auto& param : current_mode->get_params()) {
        map[param.key] = current_mode->get_param(param.key);
    }

    return map;
}

std::map<std::string, uint16_t> ModeController::get_default_params_for_mode(uint8_t mode_id) const {
    std::map<std::string, uint16_t> map;
    const auto& config = get_mode_config(mode_id);

    for (const auto& param : config.params) {
        map[param.key] = param.default_value;
    }

    return map;
}

std::map<std::string, uint16_t> ModeController::load_mode_params_from_nvs(uint8_t mode_id) const {
    std::map<std::string, uint16_t> map;
    const auto& config = get_mode_config(mode_id);

    DBG_PRINTF(ModeController, "Loading mode %u params from NVS\n", mode_id);

    for (const auto& param : config.params) {
        const std::string nvs_param_key = make_nvs_param_key(mode_id, param.key);
        const uint16_t stored_val = nvs.read<uint16>(nvs_namespace, nvs_param_key, param.default_value);
        map[param.key] = stored_val;

        DBG_PRINTF(ModeController, "   - Loaded Param [%s]: %u\n", param.key.c_str(), stored_val);
    }

    return map;
}

void ModeController::persist_mode_params_to_nvs(uint8_t mode_id) const {
    if (!current_mode) {
        return;
    }

    const auto& config = get_mode_config(mode_id);

    DBG_PRINTF(ModeController, "Persisting mode %u params to NVS\n", mode_id);

    for (const auto& param : config.params) {
        const uint16_t value = current_mode->get_param(param.key);
        const std::string nvs_param_key = make_nvs_param_key(mode_id, param.key);

        nvs.write<uint16>(nvs_namespace, nvs_param_key, value);
        DBG_PRINTF(ModeController, "   - Saved Param [%s]: %u\n", param.key.c_str(), value);
    }
}

std::string ModeController::make_nvs_param_key(uint8_t mode_id, std::string_view param_key) const {
    return "m:" + std::to_string(mode_id) + ":" + std::string(param_key);
}

uint16_t ModeController::normalize_mode_param_value(std::string_view key, int32_t value) const {
    for (const auto& param : current_mode->get_params()) {
        if (param.key != key) {
            continue;
        }

        if (key == "hue") {
            int32_t wrapped = value % 256;
            if (wrapped < 0) {
                wrapped += 256;
            }
            return static_cast<uint16_t>(wrapped);
        }

        const int32_t clamped = std::clamp<int32_t>(
            value,
            static_cast<int32_t>(param.min_value),
            static_cast<int32_t>(param.max_value)
        );

        return static_cast<uint16_t>(clamped);
    }

    const int32_t clamped = std::clamp<int32_t>(
        value,
        0,
        static_cast<int32_t>(std::numeric_limits<uint16_t>::max())
    );

    return static_cast<uint16_t>(clamped);
}