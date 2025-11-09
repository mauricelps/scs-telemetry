#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <string>
#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <atomic>

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();

    bool start(unsigned short port);
    void stop();
    bool is_running() const;

    // Fügt eine Nachricht zur Broadcast-Warteschlange hinzu (Thread-sicher)
    void queue_broadcast(const std::string& msg);

private:
    using config_t = websocketpp::config::asio;
    using server_t = websocketpp::server<config_t>;
    using connection_hdl = websocketpp::connection_hdl;

    void run_server();
    void process_message_queue();

    server_t m_server;
    std::thread m_thread;
    std::atomic<bool> m_running;

    // Verbindungen und Nachrichten-Queue
    std::mutex m_connection_mutex;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;

    std::mutex m_queue_mutex;
    std::vector<std::string> m_message_queue;

    // Handler
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, server_t::message_ptr msg);
};

// Globale Instanz
extern WebSocketServer websocket_server;