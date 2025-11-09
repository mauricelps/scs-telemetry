#include "plugin_log.hpp"
#include <fstream>
#include <mutex>
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <sstream>

static std::ofstream g_log;
static std::mutex g_log_mutex;

static std::string timestamp_now() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
       << "." << std::setw(3) << std::setfill('0') << ms.count();
    return ss.str();
}

void plugin_log_init(const std::string& path) {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (g_log.is_open()) g_log.close();
    g_log.open(path, std::ios::out | std::ios::app);
    if (g_log.is_open()) {
        g_log << timestamp_now() << " === plugin_log started ===\n";
        g_log.flush();
    }
}

void plugin_log_printf(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (!g_log.is_open()) return;
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    g_log << timestamp_now() << " " << buf << '\n';
    g_log.flush();
}

void plugin_log_flush() {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (g_log.is_open()) g_log.flush();
}

void plugin_log_close() {
    std::lock_guard<std::mutex> lk(g_log_mutex);
    if (g_log.is_open()) {
        g_log << timestamp_now() << " === plugin_log stopped ===\n";
        g_log.close();
    }
}