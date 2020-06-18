#pragma once
#include <zmq.hpp>
#include <functional>
#include <string>
#include <unordered_map>

class ZmqHttpServer {
public:
    using RequestHandler = std::function<zmq::message_t(zmq::message_t)>;

    ZmqHttpServer(const char* port)
            : _http_socket(_zmq_ctx, zmq::socket_type::stream) {
        _http_socket.bind(std::string("tcp://*:") + port);
    }

    void addRequestHandler(const char* path, RequestHandler request_handler) {
        _request_handlers[path] = std::move(request_handler);
    }

    void poll(int receive_timeout = -1, int send_timeout = 1000);

private:
    static void parsePath(char* path, size_t len, const char* request_data);

    void sendResponse(zmq::message_t& request_handle, const void* buf, int len);
    void sendResponse(zmq::message_t& request_handle, zmq::message_t& response);

    void sendMessage(zmq::message_t& msg) { _http_socket.send(msg, zmq::send_flags::sndmore); }

    zmq::context_t _zmq_ctx;
    zmq::socket_t _http_socket;
    std::unordered_map<std::string, RequestHandler> _request_handlers;
};
