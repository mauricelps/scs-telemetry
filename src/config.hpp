#pragma once

#include <string>

struct PluginConfig {
    int port = 9995;              // default
    std::string mode = "delta";   // "delta" oder "full"
    std::string ini_path_used;    // Pfad zur verwendeten INI (leer falls nicht vorhanden)
};

// Lädt die Konfiguration (liest zuerst DLL-Ordner/scs_ws_plugin.ini, dann CWD/scs_ws_plugin.ini, dann Env/Defaults)
PluginConfig load_plugin_config();

// Globale Konfiguration
extern PluginConfig g_plugin_config;