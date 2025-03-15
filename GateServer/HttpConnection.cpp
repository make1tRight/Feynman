#include "HttpConnection.h"
#include "LogicSystem.h"
#include <iostream>

HttpConnection::HttpConnection(tcp::socket socket)
     : _socket(std::move(socket)) {

}

void HttpConnection::Start() {
    auto self = shared_from_this();
    http::async_read(_socket, _buffer, _request, [self](
        beast::error_code ec, std::size_t bytes_transferred) {
        try {
            if (ec) {
                std::cout << "http read error, code: " << ec.message() << std::endl;
                return;
            }
            boost::ignore_unused(bytes_transferred);
            self->HandleRequest();
            self->CheckDeadline();
        }
        catch (std::exception& e) {
            std::cout << "HttpConnection::Start() Exception: " << ec.message() << std::endl;
        }
    });
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();
    _deadline.async_wait([self](beast::error_code ec) {
        if (!ec) {
            self->_socket.close(ec);
        }
    });
}

void HttpConnection::HandleRequest() {
    _response.version(_request.version());
    _response.keep_alive(false);

    if (_request.method() == http::verb::get) {
        bool success = LogicSystem::GetInstance()->HandleGet(
            _request.target().data(), shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    http::async_write(_socket, _response, [self](
        beast::error_code ec, std::size_t) {
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);
        self->_deadline.cancel();// 取消超时检测定时器
    });
}