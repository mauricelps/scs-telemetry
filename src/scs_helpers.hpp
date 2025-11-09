#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <optional>

// SCS SDK
#include "scssdk_telemetry.h"
#include "scssdk_value.h"

using json = nlohmann::json;

// Konvertiert scs_value_t (beliebiger Typ) zu JSON
json scsValueToJson(const scs_value_t* v);

// Kleines Hilfsmittel für Channel-Namen → JSON-Key Normalisierung (optional)
std::string normalize_channel_key(const char* name);

// Liste von Channels, die wir registrieren (breit gefächert)
const std::vector<std::pair<std::string, scs_value_type_t>>& all_channels();
