// In src/scs_context.hpp

#ifndef SCS_CONTEXT_HPP
#define SCS_CONTEXT_HPP

#include "scssdk_telemetry.h" // Sicherstellen, dass der Typ bekannt ist
#include "config.hpp"              // Sicherstellen, dass der Typ bekannt ist
#include <string>

// NUR 'extern'-Deklarationen hier. KEINE Zuweisungen.
extern const scs_telemetry_init_params_t* g_scs_params;
extern PluginConfig g_plugin_config;
extern std::string g_game_id;

#endif