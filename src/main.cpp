#include "plugin.hpp"
#include <scssdk_telemetry.h>
#include <eurotrucks2/scssdk_telemetry_eut2.h>
#include <amtrucks/scssdk_telemetry_ats.h>
#include <scssdk_telemetry_event.h>
#include <iostream>
#include <filesystem>

#include "config.hpp"
#include "websocket_server.hpp"
#include "scs_context.hpp"
#include "plugin_log.hpp"

static TelemetryPlugin plugin;
const scs_telemetry_init_params_t* g_scs_params = nullptr;
std::string g_game_id = "unknown";

#ifdef _WIN32
#include <windows.h>
#endif

static std::filesystem::path get_dll_directory() {
#ifdef _WIN32
    HMODULE hm = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&get_dll_directory), &hm)) {
        char buf[MAX_PATH];
        DWORD r = GetModuleFileNameA(hm, buf, MAX_PATH);
        if (r != 0 && r < MAX_PATH) {
            return std::filesystem::path(buf).parent_path();
        }
    }
#endif
    return std::filesystem::current_path();
}

extern "C" {
	
SCSAPI_RESULT scs_telemetry_init(const scs_u32_t version, const scs_telemetry_init_params_t *const params)
{
    if (params == nullptr) return SCS_RESULT_invalid_parameter;

    // Wir verwenden die v101-Struktur, da sie game_id enthält und kompatibel ist.
    const auto* p = static_cast<const scs_telemetry_init_params_v101_t*>(params);
    if (!p || !p->common.game_id) {
        // Fallback oder Fehler, wenn game_id nicht verfügbar ist (sehr alte SDK-Version)
        return SCS_RESULT_unsupported;
    }

    // Logging initialisieren
    try {
        auto dll_dir = get_dll_directory();
        auto log_path = (dll_dir / "plugin_debug.log").string();
        plugin_log_init(log_path);
        plugin_log_printf("scs_telemetry_init: Game: %s, Telemetry Version: %u.%u", p->common.game_id, SCS_GET_MAJOR_VERSION(version), SCS_GET_MINOR_VERSION(version));
    } catch (...) { /* Ohne Logging weiterarbeiten */ }


    // --- KORREKTE, SPIEL-SPEZIFISCHE VERSIONSPRÜFUNG ---
    const scs_u32_t game_data_version = p->common.game_version;
    plugin_log_printf("[DIAGNOSE] Prüfe Spiel-ID: '%s', Spieldaten-Version: %u.%u", p->common.game_id, SCS_GET_MAJOR_VERSION(game_data_version), SCS_GET_MINOR_VERSION(game_data_version));

    bool version_ok = false;
    if (p->common.game_id && strcmp(p->common.game_id, "eut2") == 0) {
        plugin_log_printf("[DIAGNOSE] Spiel als 'eut2' erkannt.");
        // KORREKTUR: Vergleiche die game_data_version, nicht die API-Version
        if (game_data_version >= SCS_TELEMETRY_EUT2_GAME_VERSION_1_14) {
            version_ok = true;
        } else {
            plugin_log_printf("[DIAGNOSE] ETS2 Version zu alt. Benötigt: >= 1.14");
        }
    } 
    else if (p->common.game_id && strcmp(p->common.game_id, "ats") == 0) {
        plugin_log_printf("[DIAGNOSE] Spiel als 'ats' erkannt.");
        // KORREKTUR: Vergleiche die game_data_version, nicht die API-Version
        if (game_data_version >= SCS_TELEMETRY_ATS_GAME_VERSION_1_01) {
            version_ok = true;
        } else {
            plugin_log_printf("[DIAGNOSE] ATS Version zu alt. Benötigt: >= 1.01");
        }
    }

    if (!version_ok) {
        plugin_log_printf("ERROR: Unsupported game data version for game '%s'.", p->common.game_id ? p->common.game_id : "NULL");
        return SCS_RESULT_unsupported;
    }
	
	g_game_id = p->common.game_id;
	plugin_log_printf("Game ID stored globally: %s", g_game_id.c_str());
    // --- ENDE DER VERSIONSPRÜFUNG ---


    g_scs_params = p;
    
    // Konfiguration laden
    PluginConfig cfg = load_plugin_config();
    g_plugin_config = cfg;
    plugin_log_printf("config: port=%d mode=%s ini=%s", cfg.port, cfg.mode.c_str(), cfg.ini_path_used.c_str());

    if (p->common.log) {
        std::string msg = "scs_ws_plugin: using INI at " + (cfg.ini_path_used.empty() ? "none" : cfg.ini_path_used) + ", port=" + std::to_string(cfg.port);
        p->common.log(SCS_LOG_TYPE_message, msg.c_str());
    }

    if (p->register_for_channel == nullptr || p->register_for_event == nullptr) {
        plugin_log_printf("ERROR: register functions missing in params");
        return SCS_RESULT_generic_error;
    }

    // --- Kanal-Registrierung (dein bestehender Code bleibt hier, unverändert) ---
    plugin_log_printf("Registering channels...");
    const char* float_channels[] = {
        "truck.speed", "truck.engine.rpm", "truck.cruise_control", "truck.brake.air.pressure", "truck.brake.temperature", "truck.fuel.amount",
		"truck.fuel.consumption.average", "truck.fuel.range", "truck.adblue", "truck.wear.engine", "truck.wear.transmission", "truck.wear.cabin",
		"truck.wear.chassis", "truck.wear.wheels", "truck.odometer", "truck.navigation.distance", "truck.navigation.time", "truck.navigation.speed.limit",
		"trailer.wear.body", "trailer.wear.chassis", "trailer.wear.wheels", "truck.oil.temperature", "truck.water.temperature", "truck.oil.pressure", "trailer.cargo.damage",
		"cargo.mass", "cargo.unit.mass", "rpm.limit", "adblue.capacity", "fuel.capacity"
    };
    for (const auto& channel : float_channels) {
        if (p->register_for_channel(channel, SCS_U32_NIL, SCS_VALUE_TYPE_float, 0, TelemetryPlugin::scs_on_channel_value, &plugin) == SCS_RESULT_ok) {
            plugin_log_printf("Registered float channel: %s", channel);
        } else {
            plugin_log_printf("Failed to register float channel: %s", channel);
        }
    }

    // Boolesche Werte (An/Aus)
    const char* bool_channels[] = {
        "truck.brake.parking", "truck.brake.motor", "truck.brake.air.pressure.warning", "truck.fuel.warning", "truck.adblue.warning",
		"truck.oil.pressure.warning", "truck.water.temperature.warning", "truck.electric.enabled", "truck.engine.enabled", "truck.hazard.warning",
		"truck.light.lblinker", "truck.light.rblinker", "truck.light.parking", "truck.light.beam.low", "truck.light.beam.high",
		"truck.light.beacon", "truck.differential_lock", "truck.lift_axle", "truck.trailer.lift_axle", "trailer.connected", "cargo.loaded", "is.special.job"
    };
    for (const auto& channel : bool_channels) {
        if (p->register_for_channel(channel, SCS_U32_NIL, SCS_VALUE_TYPE_bool, 0, TelemetryPlugin::scs_on_channel_value, &plugin) == SCS_RESULT_ok) {
            plugin_log_printf("Registered bool channel: %s", channel);
        } else {
            plugin_log_printf("Failed to register bool channel: %s", channel);
        }
    }
	
	//Int Werte
	const char* int_channels[] = {
		"game.time", "rest.stop", "truck.engine.gear", "truck.brake.retarder", "truck.light.aux.front", "truck.light.aux.roof", "cargo.unit.count",
		"income", "planned_distance.km", "retarder.steps"
	};
	
	for (const auto& channel : int_channels) {
		if(p->register_for_channel(channel, SCS_U32_NIL, SCS_VALUE_TYPE_s32, 0, TelemetryPlugin::scs_on_channel_value, &plugin) == SCS_RESULT_ok) {
			plugin_log_printf("Registed int channel: %s", channel);
		}else {
			plugin_log_printf("Failed to register int channel: %s", channel);
		}
	}

    // String-Werte
    const char* string_channels[] = {
        "job.source.city", "job.destination.city", "job.source.company", "job.destination.company", "cargo", "cargo.id", "destination.city.id", "destination.city", "destination.company.id",
		"destination.company", "source.city.id", "source.city", "source.company.id", "source.company", "job.market", "license.plate", "license.plate.country", "license.plate.country.id",
		"name", "brand", "body.type"
    };
    for (const auto& channel : string_channels) {
        if (p->register_for_channel(channel, SCS_U32_NIL, SCS_VALUE_TYPE_string, 0, TelemetryPlugin::scs_on_channel_value, &plugin) == SCS_RESULT_ok) {
            plugin_log_printf("Registered string channel: %s", channel);
        } else {
            plugin_log_printf("Failed to register string channel: %s", channel);
        }
    }


    // --- Event-Registrierung (mit Fehlerprüfung) ---
    plugin_log_printf("Registering for events...");

    if (p->register_for_event(SCS_TELEMETRY_EVENT_frame_end, TelemetryPlugin::scs_on_event, &plugin) != SCS_RESULT_ok) {
        plugin_log_printf("ERROR: Failed to register for SCS_TELEMETRY_EVENT_frame_end");
    }
    if (p->register_for_event(SCS_TELEMETRY_EVENT_paused, TelemetryPlugin::scs_on_event, &plugin) != SCS_RESULT_ok) {
        plugin_log_printf("ERROR: Failed to register for SCS_TELEMETRY_EVENT_paused");
    }
    if (p->register_for_event(SCS_TELEMETRY_EVENT_started, TelemetryPlugin::scs_on_event, &plugin) != SCS_RESULT_ok) {
        plugin_log_printf("ERROR: Failed to register for SCS_TELEMETRY_EVENT_started");
    }
    if (p->register_for_event(SCS_TELEMETRY_EVENT_configuration, TelemetryPlugin::scs_on_event, &plugin) != SCS_RESULT_ok) {
        plugin_log_printf("ERROR: Failed to register for SCS_TELEMETRY_EVENT_configuration");
    }
    if (p->register_for_event(SCS_TELEMETRY_EVENT_gameplay, TelemetryPlugin::scs_on_event, &plugin) != SCS_RESULT_ok) {
        plugin_log_printf("ERROR: Failed to register for SCS_TELEMETRY_EVENT_gameplay");
    }

    plugin.start();

    bool ws_started = websocket_server.start(static_cast<unsigned short>(cfg.port));
    if (ws_started) {
        plugin_log_printf("websocket started on port %d", cfg.port);
    } else {
        plugin_log_printf("websocket failed to start on port %d", cfg.port);
    }

    plugin_log_printf("scs_telemetry_init: plugin initialized successfully.");
    if (p->common.log) {
        p->common.log(SCS_LOG_TYPE_message, "scs_telemetry_init: plugin initialized");
    }
    return SCS_RESULT_ok;
}

SCSAPI_VOID scs_telemetry_shutdown(void)
{
    plugin_log_printf("scs_telemetry_shutdown: begin");
    websocket_server.stop();
    plugin.stop();
    g_scs_params = nullptr;
    plugin_log_printf("scs_telemetry_shutdown: stopping and closing log");
    plugin_log_close();
    std::cout << "[SCS Plugin] Telemetry shut down." << std::endl;
}

} // extern "C"