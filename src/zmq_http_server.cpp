#include <zmq_http_server.hpp>

void ZmqHttpServer::poll(int timeout) {
    _http_socket.set(zmq::sockopt::rcvtimeo, timeout);
    zmq::message_t request_handle;
    zmq::recv_result_t recv_result;
    recv_result = _http_socket.recv(request_handle);
    if (!recv_result || *recv_result != 5) {
        return;
    };
    zmq::message_t request;
    recv_result = _http_socket.recv(request);
    if (!recv_result || *recv_result <= 0) {
        return;
    }
    char path[128];
    parsePath(path, sizeof(path), static_cast<const char*>(request.data()));
    auto request_handler = _request_handlers.find(path);
    if (request_handler == _request_handlers.end()) {
        sendResponse404(std::move(request_handle));
        return;
    }
    auto response = request_handler->second(std::move(request));
    sendResponse(std::move(request_handle), std::move(response));
}

void ZmqHttpServer::parsePath(char* path, size_t len, const char* request_data) {
    char format[16];
    snprintf(format, sizeof(format), "GET %%%zus HTTP", len);
    if (sscanf(static_cast<const char*>(request_data), format, path) < 1) {
        return;
    }
    char* c;
    for (c = path + 1; isalnum(*c); ++c) {
    }
    *c = 0;
}

void ZmqHttpServer::sendResponse(zmq::message_t request_handle, zmq::message_t response) {
    zmq::message_t handle_view(request_handle.data(), request_handle.size(), nullptr, nullptr);
    _http_socket.send(handle_view, zmq::send_flags::sndmore);
    _http_socket.send(response, zmq::send_flags::sndmore);
    _http_socket.send(request_handle, zmq::send_flags::sndmore);
    _http_socket.send(request_handle, zmq::send_flags::none);
}

void ZmqHttpServer::sendResponse404(zmq::message_t request_handle) {
    constexpr char HTTP_404[] =
            "HTTP/1.0 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "404 Page Not Found";
    zmq::message_t error_response(const_cast<char*>(HTTP_404), sizeof(HTTP_404), nullptr, nullptr);
    sendResponse(std::move(request_handle), std::move(error_response));
}
