#pragma once

#include "connection.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_set>
#include <memory>

#include <spdlog/spdlog.h>

class Server {
public:
    Server() : _io_context(), _acceptor(_io_context), _connections({}) {
    }

    void start_accept() {
        _acceptor.async_accept([this](boost::system::error_code const err, boost::asio::ip::tcp::socket socket) {
            if (!err) {
                const auto connection = std::make_shared<Connection>(std::move(socket));
                _connections.insert(connection);

                connection->start();
            }
        });
    }

    void listen() {
        const auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8053);
        _acceptor.open(endpoint.protocol());
        _acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        _acceptor.bind(endpoint);
        _acceptor.listen();
        start_accept();
    }

    void run() {
        _io_context.run();
    }

private:
    boost::asio::io_context _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::unordered_set<std::shared_ptr<Connection>> _connections;
};
