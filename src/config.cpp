#include "config.hpp"
#include "plugin_log.hpp"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>

// Globale Konfigurationsinstanz
PluginConfig g_plugin_config;

// Hilfsfunktion, um das Verzeichnis der DLL zu bekommen
static std::filesystem::path get_dll_directory() {
    HMODULE hm = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&get_dll_directory), &hm)) {
        char buf[MAX_PATH];
        if (GetModuleFileNameA(hm, buf, MAX_PATH)) {
            return std::filesystem::path(buf).parent_path();
        }
    }
    return "";
}

// Hilfsfunktion zum Trimmen von Leerzeichen
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return str;
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

PluginConfig load_plugin_config() {
    plugin_log_printf("[Config] Loading configuration...");
    PluginConfig cfg;
    cfg.port = 9995; // Standard-Port
    cfg.mode = "default_fallback"; // Eindeutiger Standard-Modus

    std::vector<std::filesystem::path> search_paths;
    auto dll_dir = get_dll_directory();
    if (!dll_dir.empty()) {
        search_paths.push_back(dll_dir / "scs_ws_plugin.ini");
    }
    // Man könnte hier weitere Fallback-Pfade hinzufügen, z.B. das CWD
    // search_paths.push_back(std::filesystem::current_path() / "scs_ws_plugin.ini");

    for (const auto& path : search_paths) {
        plugin_log_printf("[Config] Checking for INI at: %s", path.string().c_str());
        std::ifstream file(path);
        if (file.is_open()) {
            plugin_log_printf("[Config] Found INI file at: %s", path.string().c_str());
            cfg.ini_path_used = path.string();
            std::string line;
            while (std::getline(file, line)) {
                line = trim(line);
                if (line.empty() || line[0] == '#' || line[0] == ';') {
                    continue; // Kommentare und leere Zeilen ignorieren
                }
                
                size_t equals_pos = line.find('=');
                if (equals_pos != std::string::npos) {
                    std::string key = trim(line.substr(0, equals_pos));
                    std::string value = trim(line.substr(equals_pos + 1));
                    plugin_log_printf("[Config] Read key='%s', value='%s'", key.c_str(), value.c_str());

                    if (key == "port") {
                        try {
                            cfg.port = std::stoi(value);
                        } catch (...) {
                            plugin_log_printf("[Config] WARN: Invalid port value '%s'. Using default.", value.c_str());
                        }
                    } else if (key == "mode") {
                        cfg.mode = value;
                    }
                }
            }
            plugin_log_printf("[Config] Final loaded config: port=%d, mode='%s'", cfg.port, cfg.mode.c_str());
            return cfg; // Wichtig: Beende die Suche nach dem ersten Fund
        }
    }

    plugin_log_printf("[Config] No INI file found. Using default config: port=%d, mode='%s'", cfg.port, cfg.mode.c_str());
    return cfg;
}
