#pragma once
#include <nlohmann/json.hpp>
#include <mutex>
#include <string>

using json = nlohmann::json;

class TelemetryState {
public:
    // Setzt einen Wert (normalisiert nach Key-Pfad)
    void set(const std::string& key, const json& value);

    // Vollständiger Snapshot (thread-safe)
    json snapshot();

    // Delta gegenüber letztem exportiertem Snapshot berechnen (and update last)
    json diff_and_commit();

private:
    json current_{};
    json last_exported_{};
    std::mutex mtx_;
};
