#include "websocket_server.hpp"
#include "plugin_log.hpp"
#include <iostream>
#include <chrono>

WebSocketServer::WebSocketServer() : m_running(false) {
    m_server.set_open_handler([this](connection_hdl hdl) { this->on_open(hdl); });
    m_server.set_close_handler([this](connection_hdl hdl) { this->on_close(hdl); });
    m_server.set_message_handler([this](connection_hdl hdl, server_t::message_ptr msg) { this->on_message(hdl, msg); });

    m_server.clear_access_channels(websocketpp::log::alevel::all);
    m_server.clear_error_channels(websocketpp::log::elevel::all);
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start(unsigned short port) {
    if (m_running.load()) return true;

    try {
        m_server.init_asio();
        m_server.set_reuse_addr(true);
        m_server.listen(port);
        m_server.start_accept();

        m_running.store(true);
        m_thread = std::thread(&WebSocketServer::run_server, this);

        plugin_log_printf("[WS] Server start requested, thread launched.");
        return true;
    } catch (const std::exception& e) {
        plugin_log_printf("[WS] start() exception: %s", e.what());
        m_running.store(false);
        return false;
    }
}

void WebSocketServer::stop() {
    if (!m_running.exchange(false)) {
        return; // Bereits gestoppt
    }
    plugin_log_printf("[WS] Stopping server...");

    try {
        m_server.stop_listening();

        // Alle Verbindungen schließen
        std::lock_guard<std::mutex> lock(m_connection_mutex);
        for (auto const& hdl : m_connections) {
            m_server.close(hdl, websocketpp::close::status::going_away, "Server shutdown");
        }
    } catch (const std::exception& e) {
        plugin_log_printf("[WS] Exception while closing connections: %s", e.what());
    }

    if (m_thread.joinable()) {
        m_thread.join();
    }
    plugin_log_printf("[WS] Server stopped.");
}

bool WebSocketServer::is_running() const {
    return m_running.load();
}

void WebSocketServer::queue_broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_message_queue.push_back(msg);
}

void WebSocketServer::run_server() {
    plugin_log_printf("[WS Thread] Server thread started.");
    while (m_running.load()) {
        try {
            // Abarbeitung der Server-Events (non-blocking)
            m_server.poll();

            // Nachrichten aus der Queue verarbeiten
            process_message_queue();
            
            // Kurze Pause, um CPU-Last zu vermeiden
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } catch (const std::exception& e) {
            plugin_log_printf("[WS Thread] Exception in run_server loop: %s", e.what());
        }
    }
     m_server.stop(); // Stoppt den poll-Vorgang
    plugin_log_printf("[WS Thread] Server thread finished.");
}

void WebSocketServer::process_message_queue() {
    std::vector<std::string> messages_to_send;
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_message_queue.empty()) {
            return;
        }
        messages_to_send.swap(m_message_queue);
    }

    std::lock_guard<std::mutex> lock(m_connection_mutex);
    if (m_connections.empty()) {
        return;
    }
    
    plugin_log_printf("[WS] Broadcasting %zu messages to %zu clients.", messages_to_send.size(), m_connections.size());

    for (const auto& msg : messages_to_send) {
        for (auto const& hdl : m_connections) {
            m_server.send(hdl, msg, websocketpp::frame::opcode::text);
        }
    }
}

void WebSocketServer::on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    m_connections.insert(hdl);
    plugin_log_printf("[WS] Client connected. Total clients: %zu", m_connections.size());
    m_server.send(hdl, "{\"welcome\":\"ok\"}", websocketpp::frame::opcode::text);
}

void WebSocketServer::on_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    m_connections.erase(hdl);
    plugin_log_printf("[WS] Client disconnected. Total clients: %zu", m_connections.size());
}

void WebSocketServer::on_message(connection_hdl hdl, server_t::message_ptr msg) {
    // Nicht implementiert, da wir nur senden
}

// Globale Instanz
WebSocketServer websocket_server;