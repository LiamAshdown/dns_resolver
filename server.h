#pragma once

#include <vector>
#include <limits>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include "dns.h"

using boost::asio::ip::udp;

class Server {
public:
    Server() : _io_context(), _socket(_io_context), _namespace("1.1.1.1", 53) {
    }

    void listen() {
        const udp::endpoint endpoint(udp::v4(), 8053);
        _socket.open(endpoint.protocol());
        _socket.set_option(udp::socket::reuse_address(true));
        _socket.bind(endpoint);

        spdlog::info("Listening on UDP port {}", endpoint.port());
        start_receive();
    }

    void run() {
        _io_context.run();
    }

private:
    void start_receive() {
        _buffer_reader.reset();

        _socket.async_receive_from(
            boost::asio::buffer(_buffer_reader.buffer_pointer(), MAX_PACKET_SIZE),
            _remote_endpoint,
            [this](const boost::system::error_code error, std::size_t transferred_bytes) {
                if (error) {
                    spdlog::error("Couldn't read incoming data: {}", error.message());
                } else {
                    _buffer_reader.resize(transferred_bytes);

                    handle_query();
                }

                start_receive();
            });
    }

    void handle_query() {
        try {
            const PacketDNS client_dns = DNS::parse(_buffer_reader);
            _namespace.send(DNS::build(client_dns));

            BufferReader buffer_reader = _namespace.receive();

            const PacketDNS namespace_dns = DNS::parse(buffer_reader);
            const BufferWriter buffer_writer = DNS::build(namespace_dns);

            _socket.send_to(boost::asio::buffer(buffer_writer.data(), buffer_writer.size()), _remote_endpoint);
        } catch (const std::exception &e) {
            spdlog::error("Failed to handle query: {}", e.what());
        }
    }

    boost::asio::io_context _io_context;
    udp::socket _socket;
    udp::endpoint _remote_endpoint;
    BufferReader _buffer_reader;
    Namespace _namespace;
};
