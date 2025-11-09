#include "telemetry_state.hpp"
#include <sstream>

// Utility: schreibe "a.b.c" Pfade in verschachteltes JSON
static void set_by_path(json& root, const std::string& path, const json& value) {
    std::stringstream ss(path);
    std::string token;
    json* node = &root;
    while (std::getline(ss, token, '.')) {
        if (ss.peek() == EOF) {
            (*node)[token] = value;
            return;
        }
        node = &(*node)[token];
        if (!node->is_object()) *node = json::object();
    }
}

void TelemetryState::set(const std::string& key, const json& value) {
    std::lock_guard<std::mutex> lk(mtx_);
    set_by_path(current_, key, value);
}

json TelemetryState::snapshot() {
    std::lock_guard<std::mutex> lk(mtx_);
    return current_;
}

// naive, robuste Diff (objektweise)
static void obj_diff(const json& prev, const json& curr, json& out) {
    // keys in curr
    for (auto it = curr.begin(); it != curr.end(); ++it) {
        const auto& k = it.key();
        const auto& v = it.value();
        if (!prev.contains(k)) {
            out[k] = v;
        } else {
            const auto& pv = prev.at(k);
            if (v.type() != pv.type()) {
                out[k] = v;
            } else if (v.is_object()) {
                json sub;
                obj_diff(pv, v, sub);
                if (!sub.empty()) out[k] = std::move(sub);
            } else if (v != pv) {
                out[k] = v;
            }
        }
    }
}

json TelemetryState::diff_and_commit() {
    std::lock_guard<std::mutex> lk(mtx_);
    json out = json::object();
    obj_diff(last_exported_, current_, out);
    last_exported_ = current_;
    return out;
}
