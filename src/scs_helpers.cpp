#include "scs_helpers.hpp"
#include <algorithm>
#include <cctype>

json scsValueToJson(const scs_value_t* v) {
    if (!v) return nullptr;
    switch (v->type) {
        case SCS_VALUE_TYPE_bool:
            return v->value_bool.value != 0;
        case SCS_VALUE_TYPE_s32:
            return v->value_s32.value;
        case SCS_VALUE_TYPE_u32:
            return v->value_u32.value;
        case SCS_VALUE_TYPE_s64:
            return static_cast<int64_t>(v->value_s64.value);
        case SCS_VALUE_TYPE_u64:
            return static_cast<uint64_t>(v->value_u64.value);
        case SCS_VALUE_TYPE_float:
            return v->value_float.value;
        case SCS_VALUE_TYPE_double:
            return v->value_double.value;
        case SCS_VALUE_TYPE_string:
            return v->value_string.value ? std::string(v->value_string.value) : "";
        case SCS_VALUE_TYPE_fvector: {
            json o;
            o["x"] = v->value_fvector.x;
            o["y"] = v->value_fvector.y;
            o["z"] = v->value_fvector.z;
            return o;
        }
        case SCS_VALUE_TYPE_dplacement: {
            json o;
            o["x"] = v->value_dplacement.position.x;
            o["y"] = v->value_dplacement.position.y;
            o["z"] = v->value_dplacement.position.z;
            o["heading"] = v->value_dplacement.orientation.heading;
            o["pitch"]   = v->value_dplacement.orientation.pitch;
            o["roll"]    = v->value_dplacement.orientation.roll;
            return o;
        }
        case SCS_VALUE_TYPE_fplacement: {
            json o;
            o["x"] = v->value_fplacement.position.x;
            o["y"] = v->value_fplacement.position.y;
            o["z"] = v->value_fplacement.position.z;
            o["heading"] = v->value_fplacement.orientation.heading;
            o["pitch"]   = v->value_fplacement.orientation.pitch;
            o["roll"]    = v->value_fplacement.orientation.roll;
            return o;
        }
        case SCS_VALUE_TYPE_euler: {
            json o;
            o["heading"] = v->value_euler.heading;
            o["pitch"]   = v->value_euler.pitch;
            o["roll"]    = v->value_euler.roll;
            return o;
        }
#ifdef SCS_VALUE_TYPE_fmatrix
        case SCS_VALUE_TYPE_fmatrix:
            // Falls dein SDK diese Konstante enthält, kannst du hier eine passende Wandlung ergänzen.
            return nullptr;
#endif
        default:
            return nullptr; // nicht unterstützt / selten gebraucht
    }
}

std::string normalize_channel_key(const char* name) {
    if (!name) return "";
    std::string s(name);
    std::replace(s.begin(), s.end(), '.', '_');
    std::replace(s.begin(), s.end(), ' ', '_');
    return s;
}

// Eine breite, praxiserprobte Auswahl gängiger Channels (nicht vollständig – aber sehr umfangreich)
const std::vector<std::pair<std::string, scs_value_type_t>>& all_channels() {
    static std::vector<std::pair<std::string, scs_value_type_t>> ch = {
        // Truck dynamics
        {"truck.speed", SCS_VALUE_TYPE_float},
        {"truck.engine.rpm", SCS_VALUE_TYPE_float},
        {"truck.engine.gear", SCS_VALUE_TYPE_s32},
        {"truck.engine.gear_display", SCS_VALUE_TYPE_string},
        {"truck.engine.enabled", SCS_VALUE_TYPE_bool},
        {"truck.fuel.amount", SCS_VALUE_TYPE_float},
        {"truck.fuel.capacity", SCS_VALUE_TYPE_float},
        {"truck.fuel.range", SCS_VALUE_TYPE_float},
        {"truck.fuel.avg_consumption", SCS_VALUE_TYPE_float},
        {"truck.oil.temperature", SCS_VALUE_TYPE_float},
        {"truck.coolant.temperature", SCS_VALUE_TYPE_float},
        {"truck.brake.air_pressure", SCS_VALUE_TYPE_float},
        {"truck.brake.temperature", SCS_VALUE_TYPE_float},
        {"truck.battery.voltage", SCS_VALUE_TYPE_float},
        {"truck.adblue", SCS_VALUE_TYPE_float},
        {"truck.odometer", SCS_VALUE_TYPE_double},

        // Drive controls / inputs
        {"truck.input.steering", SCS_VALUE_TYPE_float},
        {"truck.input.throttle", SCS_VALUE_TYPE_float},
        {"truck.input.brake", SCS_VALUE_TYPE_float},
        {"truck.input.clutch", SCS_VALUE_TYPE_float},
        {"truck.parking.brake", SCS_VALUE_TYPE_bool},
        {"truck.motor.brake", SCS_VALUE_TYPE_bool},
        {"truck.retarder.level", SCS_VALUE_TYPE_s32},
        {"truck.cruise_control", SCS_VALUE_TYPE_bool},
        {"truck.cruise_control.speed", SCS_VALUE_TYPE_float},
        {"truck.wipers", SCS_VALUE_TYPE_bool},
        {"truck.lights.beam_low", SCS_VALUE_TYPE_bool},
        {"truck.lights.beam_high", SCS_VALUE_TYPE_bool},
        {"truck.lights.beacon", SCS_VALUE_TYPE_bool},
        {"truck.lights.parking", SCS_VALUE_TYPE_bool},
        {"truck.lights.brake", SCS_VALUE_TYPE_bool},
        {"truck.blinker.left", SCS_VALUE_TYPE_bool},
        {"truck.blinker.right", SCS_VALUE_TYPE_bool},
        {"truck.warning.hazard", SCS_VALUE_TYPE_bool},

        // Placement & motion
        {"truck.world.placement", SCS_VALUE_TYPE_fplacement},
        {"truck.cabin.placement", SCS_VALUE_TYPE_fplacement},
        {"truck.acceleration.linear", SCS_VALUE_TYPE_fvector},
        {"truck.angular_velocity", SCS_VALUE_TYPE_fvector},

        // Damage
        {"truck.damage.engine", SCS_VALUE_TYPE_float},
        {"truck.damage.transmission", SCS_VALUE_TYPE_float},
        {"truck.damage.cabin", SCS_VALUE_TYPE_float},
        {"truck.damage.chassis", SCS_VALUE_TYPE_float},
        {"truck.damage.wheels", SCS_VALUE_TYPE_float},

        // Navigation & route
        {"navigation.distance", SCS_VALUE_TYPE_float},
        {"navigation.time", SCS_VALUE_TYPE_float},
        {"navigation.speed.limit", SCS_VALUE_TYPE_float},

        // Trailer
        {"trailer.attached", SCS_VALUE_TYPE_bool},
        {"trailer.name", SCS_VALUE_TYPE_string},
        {"trailer.mass", SCS_VALUE_TYPE_float},
        {"trailer.damage.chassis", SCS_VALUE_TYPE_float},
        {"trailer.damage.wheels", SCS_VALUE_TYPE_float},
        {"trailer.world.placement", SCS_VALUE_TYPE_fplacement},

        // Job / cargo
        {"job.delivery.time_remaining", SCS_VALUE_TYPE_u32},
        {"job.cargo.name", SCS_VALUE_TYPE_string},
        {"job.cargo.mass", SCS_VALUE_TYPE_float},
        {"job.source.city", SCS_VALUE_TYPE_string},
        {"job.source.company", SCS_VALUE_TYPE_string},
        {"job.destination.city", SCS_VALUE_TYPE_string},
        {"job.destination.company", SCS_VALUE_TYPE_string},
        {"job.income", SCS_VALUE_TYPE_u64},

        // Game / world
        {"game.paused", SCS_VALUE_TYPE_bool},
        {"game.time", SCS_VALUE_TYPE_u32},
        {"game.scale", SCS_VALUE_TYPE_float},
        {"world.weather.rain", SCS_VALUE_TYPE_float},
        {"world.temperature", SCS_VALUE_TYPE_float},

        // UI-like
        {"ui.gear.suggested", SCS_VALUE_TYPE_s32},
        {"ui.speed", SCS_VALUE_TYPE_float},
        {"ui.speed_unit", SCS_VALUE_TYPE_string},
    };
    return ch;
}