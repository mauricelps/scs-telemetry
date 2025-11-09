#pragma once

#include <scssdk_telemetry.h>
#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <mutex>
#include <chrono>

class TelemetryPlugin {
public:
    void start();
    void stop();

    // Callback-Implementierungen
    void on_channel_value(const char* name, const scs_u32_t index, const scs_value_t* value);
    void on_event(const scs_event_t event, const void* event_info);
    void on_frame_end();
	void broadcast_message(const std::string& message);

    // Statische Wrapper für die C-API des Spiels
    static void scs_on_channel_value(const scs_string_t name, const scs_u32_t index, const scs_value_t* value, const scs_context_t context);
    static void scs_on_event(const scs_event_t event, const void* event_info, scs_context_t context);
	
	void clear_job_data();

private:
    bool running = false;

    // Thread-sichere Maps zum Speichern des Telemetrie-Zustands
    std::mutex state_mutex;
    std::map<std::string, nlohmann::json> current_telemetry_state;
    std::map<std::string, nlohmann::json> last_sent_telemetry_state; // Für den Delta-Modus

    // Zeitsteuerung für den Devenv-Modus
    std::chrono::steady_clock::time_point last_devenv_send_time;
};