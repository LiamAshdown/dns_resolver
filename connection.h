#pragma once

#include <boost/asio.hpp>

#include "buffer-reader.h"
#include "spdlog/spdlog.h"
#include "dns.h"

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(boost::asio::ip::tcp::socket socket) : _socket(std::move(socket)), _buffer_reader() {
    }

    void start() {
        async_read();
    }

    void async_read() {
        _socket.async_read_some(boost::asio::buffer(_buffer_reader.buffer_pointer(), MAX_PACKET_SIZE),
            std::bind(&Connection::read_handler, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }

private:
    void read_handler(boost::system::error_code error, std::size_t transferred_bytes) {

        if (error) {
            spdlog::error("Couldn't read incoming data: {}", error.message());

            async_read();

            return;
        }

        DNS dns;
        dns.parse(_buffer_reader);

        async_read();
    }

    boost::asio::ip::tcp::socket _socket;
    BufferReader _buffer_reader;
};
