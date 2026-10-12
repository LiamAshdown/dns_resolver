#pragma once

#include <vector>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include "cache.h"
#include "dns.h"

using boost::asio::ip::udp;

struct Request {
    uint16_t client_id;
    CacheKey key;
    BufferReader client_reader;
    udp::endpoint client;

    udp::socket upstream;
    udp::endpoint upstream_sender;
    BufferWriter upstream_writer;
    BufferReader upstream_reader;

    BufferWriter response;
    boost::asio::steady_timer timeout;

    explicit Request(boost::asio::io_context &io)
        : upstream(io, udp::endpoint(udp::v4(), 0)), timeout(io) {}
};

class Server {
public:
    Server() : _io_context(), _socket(_io_context), _upstream_endpoint(boost::asio::ip::make_address("1.1.1.1"), 53) {
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
        auto request = std::make_shared<Request>(_io_context);

        _socket.async_receive_from(
            boost::asio::buffer(request->client_reader.buffer_pointer(), MAX_PACKET_SIZE), request->client,
            [this, request](const boost::system::error_code &error_code, const std::size_t bytes) {
                start_receive();

                if (error_code) {
                    return;
                }

                request->client_reader.resize(bytes);
                forward(request);
            });
    }

    void forward(std::shared_ptr<Request> request) {
        try {
            DNSPayload query = DNS::parse(request->client_reader);
            request->client_id = query.header.id;
            request->key = _cache.make_key(query.questions.at(0));

            if (_cache.has_key(request->key)) {
                CacheEntry* cache_entry = _cache.get_cache(request->key);

                auto now = std::chrono::steady_clock::now();

                if (now < cache_entry->expires) {
                    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - cache_entry->stored).count();
                    for (auto& it : cache_entry->payload.answers) {
                        it.ttl -= static_cast<uint32_t>(elapsed);
                    }

                    query = cache_entry->payload;

                    request->response = DNS::build(query);

                    _socket.async_send_to(boost::asio::buffer(request->response.data(), request->response.size()),
                      request->client, [request](auto, auto) {});

                    return;
                }

                _cache.remove_cache(request->key);
            }

            request->upstream_writer = DNS::build(DNS::parse(request->client_reader));
        } catch (const std::exception &e) {
            spdlog::error("Bad query: {}", e.what());
            return;
        }

        request->upstream.async_send_to(
            boost::asio::buffer(request->upstream_writer.data(), request->upstream_writer.size()), _upstream_endpoint,
            [this, request](const boost::system::error_code &error_code, std::size_t) {
                if (error_code) {
                    return;
                }

                request->timeout.expires_after(std::chrono::seconds(2));
                request->timeout.async_wait([request](const boost::system::error_code &error_code) {
                    if (!error_code) {
                        request->upstream.cancel();
                    }
                });

                request->upstream.async_receive_from(
                    boost::asio::buffer(request->upstream_reader.buffer_pointer(), MAX_PACKET_SIZE), request->upstream_sender,
                    [this, request](const boost::system::error_code &error_code, std::size_t bytes) {
                        request->timeout.cancel();
                        if (error_code) {
                            return;
                        }

                        request->upstream_reader.resize(bytes);
                        reply(request);
                    });
            });
    }

    void reply(std::shared_ptr<Request> request) {
        try {
            const DNSPayload answer = DNS::parse(request->upstream_reader);

            if (!answer.answers.empty()) {
                _cache.add_key(request->key, answer);
            }

            request->response = DNS::build(answer);
        } catch (const std::exception &e) {
            spdlog::error("Bad upstream reply: {}", e.what());
            return;
        }

        _socket.async_send_to(
            boost::asio::buffer(request->response.data(), request->response.size()), request->client,
            [request](boost::system::error_code, std::size_t) {

            });
    }

    boost::asio::io_context _io_context;
    udp::socket _socket;
    udp::endpoint _upstream_endpoint;
    Cache _cache;
};
