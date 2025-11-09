#pragma once
#include <string>

void plugin_log_init(const std::string& path);
void plugin_log_printf(const char* fmt, ...);
void plugin_log_flush();
void plugin_log_close();