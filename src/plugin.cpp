#include "plugin.hpp"
#include "config.hpp" // Wir brauchen Zugriff auf g_plugin_config
#include "scs_context.hpp"
#include "websocket_server.hpp"
#include "plugin_log.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cmath>
#include <exception>
#include <vector>

// Hilfsfunktion zur Konvertierung von scs_value_t in nlohmann::json (unverändert)
static nlohmann::json scs_value_to_json(const std::string& channel_name, const scs_value_t* v) {
    if (!v) return nullptr;
    if (channel_name == "truck.speed" && v->type == SCS_VALUE_TYPE_float) {
        float speed_ms = v->value_float.value;
        return (std::abs(speed_ms) < 0.1) ? 0.0 : speed_ms * 3.6;
    }
    switch (v->type) {
        case SCS_VALUE_TYPE_bool: return (v->value_bool.value != 0);
        case SCS_VALUE_TYPE_s32: return v->value_s32.value;
        case SCS_VALUE_TYPE_u32: return v->value_u32.value;
        case SCS_VALUE_TYPE_u64: return v->value_u64.value;
        case SCS_VALUE_TYPE_s64: return v->value_s64.value;
        case SCS_VALUE_TYPE_float: return v->value_float.value;
        case SCS_VALUE_TYPE_double: return v->value_double.value;
        case SCS_VALUE_TYPE_string: return v->value_string.value ? std::string(v->value_string.value) : "";
        default: return nullptr;
    }
}

// on_channel_value (unverändert)
void TelemetryPlugin::on_channel_value(const char* name, const scs_u32_t index, const scs_value_t* value) {
    if (!name) return;
    try {
        std::string channel_name = name;
        if (index != SCS_U32_NIL) {
            channel_name += "[" + std::to_string(index) + "]";
        }
        nlohmann::json val = scs_value_to_json(channel_name, value);
        std::lock_guard<std::mutex> lock(state_mutex);
        current_telemetry_state[channel_name] = val;
    } catch (const std::exception& e) {
        plugin_log_printf("[PLUGIN] Exception in on_channel_value: %s", e.what());
    }
}

// on_event (unverändert, die wichtige Logik hier drin ist korrekt)
void TelemetryPlugin::on_event(const scs_event_t event, const void* event_info) {
    try {
        plugin_log_printf("[DIAGNOSE] Event empfangen, Typ: %d", event);

        if (event == SCS_TELEMETRY_EVENT_configuration && event_info) {
            const auto* config_event = static_cast<const scs_telemetry_configuration_t*>(event_info);
            if (config_event->id) {
                plugin_log_printf("[DIAGNOSE] Configuration Event ID: %s", config_event->id);
            }
            std::string config_id = config_event->id;
            std::lock_guard<std::mutex> lock(state_mutex);
            for (const scs_named_value_t* attr = config_event->attributes; attr && attr->name; ++attr) {
                std::string key = config_id + "." + attr->name;
                current_telemetry_state[key] = scs_value_to_json(key, &attr->value);
            }
            return;
        }

        if (event == SCS_TELEMETRY_EVENT_gameplay && event_info) {
            const auto* gameplay_event = static_cast<const scs_telemetry_gameplay_event_t*>(event_info);
            if (!gameplay_event || !gameplay_event->id) {
                return;
            }
            plugin_log_printf("[DIAGNOSE] Gameplay Event ID: %s", gameplay_event->id);
            
            nlohmann::json event_json;
            event_json["type"] = "gameplay";
            event_json["event_name"] = gameplay_event->id;
            nlohmann::json attributes_json;
            for (const scs_named_value_t* attr = gameplay_event->attributes; attr && attr->name; ++attr) {
                attributes_json[attr->name] = scs_value_to_json(attr->name, &attr->value);
            }
            if (!attributes_json.empty()) {
                event_json["attributes"] = attributes_json;
            }
            websocket_server.queue_broadcast(event_json.dump());

            if (strcmp(gameplay_event->id, "job.delivered") == 0 || strcmp(gameplay_event->id, "job.cancelled") == 0) {
                clear_job_data();
            }
            return;
        }

        if (event == SCS_TELEMETRY_EVENT_frame_end) {
            on_frame_end();
        }

    } catch (const std::exception& e) {
        plugin_log_printf("[PLUGIN] Exception in on_event: %s", e.what());
    }
}

// on_frame_end (mit Korrektur)
void TelemetryPlugin::on_frame_end() {
    try {
        std::lock_guard<std::mutex> lock(state_mutex);

        if (g_plugin_config.mode == "devenv") {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_devenv_send_time) < std::chrono::seconds(1)) {
                return;
            }
            last_devenv_send_time = now;
            nlohmann::json frame_data = current_telemetry_state;
            frame_data["timestamp"] = std::time(nullptr);
            frame_data["game"] = g_game_id;
            websocket_server.queue_broadcast(frame_data.dump());
            return;
        }

        if (g_plugin_config.mode == "delta") {
            nlohmann::json delta_data;
            // ================== HIER IST DIE KORREKTUR ==================
            // Der Iterator eines JSON-Objekts wird wie ein std::map-Iterator behandelt.
            // Zugriff erfolgt über it->first für den Schlüssel und it->second für den Wert.
            for (auto it = current_telemetry_state.cbegin(); it != current_telemetry_state.cend(); ++it) {
                if (last_sent_telemetry_state.find(it->first) == last_sent_telemetry_state.end() || last_sent_telemetry_state[it->first] != it->second) {
                    delta_data[it->first] = it->second;
                }
            }
            // ==========================================================

            if (!delta_data.empty()) {
                delta_data["timestamp"] = std::time(nullptr);
                delta_data["game"] = g_game_id;
                websocket_server.queue_broadcast(delta_data.dump());
            }
            last_sent_telemetry_state = current_telemetry_state;
            return;
        }

        // FULL-Modus (Fallback)
        nlohmann::json frame_data = current_telemetry_state;
        frame_data["timestamp"] = std::time(nullptr);
        frame_data["game"] = g_game_id;
        websocket_server.queue_broadcast(frame_data.dump());

    } catch (const std::exception& e) {
        plugin_log_printf("[PLUGIN] Exception in on_frame_end: %s", e.what());
    }
}

// clear_job_data (mit Korrektur)
void TelemetryPlugin::clear_job_data() {
    plugin_log_printf("[PLUGIN] clear_job_data: Removing internal job and cargo data");
    
    std::lock_guard<std::mutex> lock(state_mutex);
    std::vector<std::string> keys_to_remove;
    
    // ================== HIER IST DIE ZWEITE KORREKTUR ==================
    for (auto it = current_telemetry_state.cbegin(); it != current_telemetry_state.cend(); ++it) {
        const std::string& key = it->first; // Korrekt: it->first ist der Schlüssel
        if (key.rfind("job.", 0) == 0 || key.rfind("cargo.", 0) == 0) {
            keys_to_remove.push_back(key);
        }
    }
    // ===============================================================

    for (const auto& key : keys_to_remove) {
        current_telemetry_state.erase(key);
    }
    
    plugin_log_printf("[PLUGIN] %zu job-bezogene Schlüssel aus dem internen Zustand entfernt.", keys_to_remove.size());
}

// --- Start/Stop und statische Wrapper (unverändert) ---
void TelemetryPlugin::start() {
    running = true;
    last_devenv_send_time = std::chrono::steady_clock::now();
    plugin_log_printf("TelemetryPlugin started with multi-mode support");
}
void TelemetryPlugin::stop() {
    running = false;
    plugin_log_printf("TelemetryPlugin stopped");
}
void TelemetryPlugin::scs_on_channel_value(const scs_string_t name, const scs_u32_t index, const scs_value_t* value, const scs_context_t context) {
    TelemetryPlugin* self = static_cast<TelemetryPlugin*>(context);
    if (self) self->on_channel_value(name, index, value);
}
void TelemetryPlugin::scs_on_event(const scs_event_t event, const void* event_info, const scs_context_t context) {
    TelemetryPlugin* self = static_cast<TelemetryPlugin*>(context);
    if (self) self->on_event(event, event_info);
}