#include <zmq_http_server.hpp>

static constexpr char HTTP_404[] =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "404 Page Not Found";

void ZmqHttpServer::poll(int timeout) {
    _timers.execute();

    // receive request handle
    zmq::recv_result_t recv_result;
    zmq::message_t request_handle;
    timeout = std::min<int>(timeout, _timers.timeout() & 0x7FFFFFFF);
    _http_socket.set(zmq::sockopt::rcvtimeo, timeout);
    recv_result = _http_socket.recv(request_handle);
    if (!recv_result || *recv_result != 5) {
        return;
    };

    // receive request
    zmq::message_t request;
    timeout = std::min<int>(timeout, _timers.timeout() & 0x7FFFFFFF);
    _http_socket.set(zmq::sockopt::rcvtimeo, timeout);
    recv_result = _http_socket.recv(request);
    if (!recv_result || *recv_result <= 0) {
        return;
    }

    // parse url path
    char path[128];
    parsePath(path, sizeof(path), static_cast<const char*>(request.data()));
    auto request_handler = _request_handlers.find(path);
    if (request_handler == _request_handlers.end()) {
        sendResponse(request_handle, HTTP_404, sizeof(HTTP_404));
        return;
    }

    // create and send response
    auto response = request_handler->second(std::move(request));
    sendResponse(request_handle, response);
}

void ZmqHttpServer::parsePath(char* path, size_t len, const char* request_data) {
    char format[16];
    snprintf(format, sizeof(format), "GET %%%zus HTTP", len);
    if (sscanf(static_cast<const char*>(request_data), format, path) < 1) {
        return;
    }
    char* c = path + 1;
    while (isalnum(*c)) {
        ++c;
    }
    *c = 0;
}

void ZmqHttpServer::sendResponse(zmq::message_t& request_handle, zmq::message_t& response) {
    zmq::message_t response_handle(request_handle.data(), request_handle.size(), nullptr, nullptr);
    sendMessage(response_handle);
    sendMessage(response);
    sendMessage(request_handle);
    sendMessage(request_handle);
}

void ZmqHttpServer::sendResponse(zmq::message_t& request_handle, const void* buf, int len) {
    zmq::message_t response(const_cast<void*>(buf), len, nullptr, nullptr);
    sendResponse(request_handle, response);
}
