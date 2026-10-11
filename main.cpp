#include <memory>
#include <vector>
#include <iostream>
#include <array>
#include <ranges>
#include <map>


#include <boost/asio.hpp>
#include "server.h"

// class BufferReader {
// public:
//     BufferReader(std::vector<char> buffer) : _buffer(std::move(buffer)), _offset(0) {
//     }
//
//     template<typename T>
//     T read() {
//         const std::size_t size = sizeof(T);
//
//         if (_offset + size > _buffer.size()) {
//             throw std::runtime_error("Buffer overflow");
//         }
//
//         T val;
//         std::memcpy(&val, _buffer.data() + _offset, size);
//         _offset += size;
//
//         return val;
//     }
//
//     std::string read_string(const int length) {
//         if (_offset + length > _buffer.size()) {
//             throw std::runtime_error("Buffer overflow");
//         }
//
//         std::string result(_buffer.data() + _offset, length);
//
//         _offset += length;
//
//         return result;
//     }
//
//     size_t offset() const { return _offset; }
//     const std::vector<char>& buffer() const { return _buffer; }
//     void seek(std::size_t offset) { _offset = offset; }
//
//     std::vector<char> _buffer;
//     std::size_t _offset;
// };

std::string parse_domain_name(BufferReader& reader) {
    std::string domain_name = "";

    bool jumped = false;
    std::size_t original_offset = 0;

    while (true) {

        uint8_t length_byte = reader.read<uint8_t>();

        if (length_byte == 0) {
            break;
        }

        // First two bits are the compression flag
        if ((length_byte & 0b11000000) == 0b11000000) {
            uint8_t second_byte = reader.read<uint8_t>();
            // merge last 6 bits and second byte bits to get the offset
            uint16_t pointer_bytes = ((length_byte & 0b00111111) << 8) | second_byte;

            if (!jumped) {
                original_offset = reader.offset();
                jumped = true;
            }

            reader.seek(pointer_bytes);
            continue;
        }

        if (!domain_name.empty()) {
            domain_name += ".";
        }

        domain_name += reader.read_string(length_byte);
    }

    if (jumped) {
        reader.seek(original_offset);
    }

    return domain_name;
}

void encode_domain_name(std::string domain_name, BufferWriter& writer) {
    std::string encoding = "";
    for (auto token : std::views::split(domain_name, '.')) {
        writer.write<uint8_t>(token.size());
        writer.write_string(std::string(token.begin(), token.end()));
    }

    // null terminator
    writer.write<uint8_t>(0);
}

int main() {

    Server server;
    server.listen();
    server.run();

//     boost::asio::io_context io_context;
//
//     boost::asio::ip::udp::socket socket(
//         io_context,
//         boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 8053)
//     );
//
//     for (;;) {
//         std::vector<char> receive_buffer(1024);
//         boost::asio::ip::udp::endpoint remote_endpoint;
//         socket.receive_from(
//             boost::asio::buffer(receive_buffer),
//             remote_endpoint
//         );
//
//         BufferReader reader(receive_buffer);
//
//         DNSHeader header{
//             .id = ntohs(reader.read<uint16_t>()),
//             .flags = ntohs(reader.read<uint16_t>()), // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
//             .qdcount = ntohs(reader.read<uint16_t>()),
//             .ancount = ntohs(reader.read<uint16_t>()),
//             .nscount = ntohs(reader.read<uint16_t>()),
//             .arcount = ntohs(reader.read<uint16_t>())
//         };
//
//         std::vector<DNSQuestion> questions;
//         for (uint16_t i = 0; i < header.qdcount; ++i) {
//             DNSQuestion q;
//             q.qname = parse_domain_name(reader);
//             q.qtype = ntohs(reader.read<uint16_t>());
//             q.qclass = ntohs(reader.read<uint16_t>());
//             questions.push_back(q);
//         }
//
//         std::vector<DNSResourceRecord> answers;
//         for (uint16_t i = 0; i < header.ancount; ++i) {
//             DNSResourceRecord record;
//             record.name = parse_domain_name(reader);
//             record.type = ntohs(reader.read<uint16_t>());
//             record.class_type = ntohs(reader.read<uint16_t>());
//             record.ttl = ntohl(reader.read<uint32_t>());
//             record.rd_length = ntohs(reader.read<uint16_t>());
//
//             for (uint16_t j = 0; j < record.rd_length; ++j) {
//                 record.rdata.push_back(reader.read<uint8_t>());
//             }
//             answers.push_back(record);
//         }
//
//         if (questions.empty()) {
//             continue;
//         }
//
//         BufferWriter writer;
//
//         writer.write<uint16_t>(12);
//         writer.write<uint16_t>(htons(0b000000100000000));
//         writer.write<uint16_t>(htons(1));
//         writer.write<uint16_t>(htons(0));
//         writer.write<uint16_t>(htons(0));
//         writer.write<uint16_t>(htons(0));
//
//         encode_domain_name(questions[0].qname,writer);
//         writer.write<uint16_t>(htons(1));
//         writer.write<uint16_t>(htons(1));
//
//
//         boost::asio::io_context client_io_context;
//         const std::string nameserver = "1.1.1.1";
//         const std::string port = "53";
//
//         // Resolve nameserver endpoint
//         boost::asio::ip::udp::resolver resolver(io_context);
//         boost::asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(boost::asio::ip::udp::v4(), nameserver, port).begin();
//
//         boost::asio::ip::udp::socket socket(io_context);
//         socket.open(boost::asio::ip::udp::v4());
//
//
//         std::size_t bytes_sent = socket.send_to(
//             boost::asio::buffer(writer.data(), writer.size()),
//             receiver_endpoint
//         );
//
//         std::vector<char> recv_buf(512);
//         boost::asio::ip::udp::endpoint sender_endpoint;
//
//         std::size_t bytes_received = socket.receive_from(
//             boost::asio::buffer(recv_buf),
//             sender_endpoint
//         );
//
//         int test =0;
//
//         BufferReader readerRcv(recv_buf);
//
//         DNSHeader headerRcv{
//             .id = ntohs(readerRcv.read<uint16_t>()),
//             .flags = ntohs(readerRcv.read<uint16_t>()), // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
//             .qdcount = ntohs(readerRcv.read<uint16_t>()),
//             .ancount = ntohs(readerRcv.read<uint16_t>()),
//             .nscount = ntohs(readerRcv.read<uint16_t>()),
//             .arcount = ntohs(readerRcv.read<uint16_t>())
//         };
//
//         std::vector<DNSQuestion> questionsRecv;
//         for (uint16_t i = 0; i < headerRcv.qdcount; ++i) {
//             DNSQuestion q;
//             q.qname = parse_domain_name(readerRcv);
//             q.qtype = ntohs(readerRcv.read<uint16_t>());
//             q.qclass = ntohs(readerRcv.read<uint16_t>());
//             questions.push_back(q);
//         }
//
//         std::vector<DNSResourceRecord> answersRecv;
//         for (uint16_t i = 0; i < headerRcv.ancount; ++i) {
//             DNSResourceRecord record;
//             record.name = parse_domain_name(readerRcv);
//             record.type = ntohs(readerRcv.read<uint16_t>());
//             record.class_type = ntohs(readerRcv.read<uint16_t>());
//             record.ttl = ntohl(readerRcv.read<uint32_t>());
//             record.rd_length = ntohs(readerRcv.read<uint16_t>());
//
//             for (uint16_t j = 0; j < record.rd_length; ++j) {
//                 record.rdata.push_back(readerRcv.read<uint8_t>());
//             }
//
//             boost::asio::ip::address_v4::bytes_type ip_bytes;
//             std::copy(recv_buf.begin() + readerRcv.offset(), recv_buf.begin() + readerRcv.offset() + 4, ip_bytes.begin());
//             boost::asio::ip::address_v4 ip(ip_bytes);
//
//             std::string ip_str = ip.to_string();
//
//             answersRecv.push_back(record);
//         }
//
//     }

    return 0;
}
