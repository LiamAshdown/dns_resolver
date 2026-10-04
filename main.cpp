#include <memory>
#include <vector>
#include <iostream>
#include <array>
#include <ranges>
#include <map>

#include <boost/asio.hpp>


// https://www.rfc-editor.org/info/rfc1035/#section-3.2.2
enum Types : uint8_t {
    A = 1, // a host address

    NS = 2, // an authoritative name server

    MD = 3, // a mail destination (Obsolete - use MX)

    MF = 4, // a mail forwarder (Obsolete - use MX)

    CNAME = 5, // the canonical name for an alias

    SOA = 6, // marks the start of a zone of authority

    MB = 7, // a mailbox domain name (EXPERIMENTAL)

    MG = 8, // a mail group member (EXPERIMENTAL)

    MR = 9, // a mail rename domain name (EXPERIMENTAL)

    NLL = 10, // a null RR (EXPERIMENTAL)

    WKS = 11, // a well known service description

    PTR = 12, // a domain name pointer

    HINFO = 13, // host information

    MINFO = 14, // mailbox or mail list information

    MX = 15, // mail exchange

    TXT = 16 // text strings
};

enum Classes : uint8_t {
    INTE = 1, // the Internet
    CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
    CH = 3, // the CHAOS class
    HS = 4 // Hesiod [Dyer 87]
};


//
// 1  1  1  1  1  1
// 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                      ID                       |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    QDCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    ANCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    NSCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                    ARCOUNT                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

// 1  1  1  1  1  1
// 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                                               |
// /                     QNAME                     /
// /                                               /
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                     QTYPE                     |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                     QCLASS                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
struct DNSQuestion {
    std::string qname;
    uint16_t qtype;
    uint16_t qclass;
};

// 1  1  1  1  1  1
// 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                                               |
// /                                               /
// /                      NAME                     /
// |                                               |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                      TYPE                     |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                     CLASS                     |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                      TTL                      |
// |                                               |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |                   RDLENGTH                    |
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
// /                     RDATA                     /
// /                                               /
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
struct DNSResourceRecord {
    std::string name;
    uint16_t type;
    uint16_t class_type;
    uint32_t ttl;
    int16_t rd_length;
    std::vector<uint8_t> rdata;
};

class BufferReader {
public:
    BufferReader(std::vector<char> buffer) : _buffer(std::move(buffer)), _offset(0) {
    }

    template<typename T>
    T read() {
        const std::size_t size = sizeof(T);

        if (_offset + size > _buffer.size()) {
            throw std::runtime_error("Buffer overflow");
        }

        T val;
        std::memcpy(&val, _buffer.data() + _offset, size);
        _offset += size;

        return val;
    }

    std::string read_string(const int length) {
        if (_offset + length > _buffer.size()) {
            throw std::runtime_error("Buffer overflow");
        }

        std::string result(_buffer.data() + _offset, length);

        _offset += length;

        return result;
    }

    size_t offset() const { return _offset; }
    const std::vector<char>& buffer() const { return _buffer; }
    void seek(std::size_t offset) { _offset = offset; }

    std::vector<char> _buffer;
    std::size_t _offset;
};

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

std::string encode_domain_name(std::string domain_name) {

    for (auto token : std::views::split(domain_name, ',')


}

int main() {
    boost::asio::io_context io_context;

    boost::asio::ip::udp::socket socket(
        io_context,
        boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 8053)
    );

    for (;;) {
        std::vector<char> receive_buffer(1024);
        boost::asio::ip::udp::endpoint remote_endpoint;
        socket.receive_from(
            boost::asio::buffer(receive_buffer),
            remote_endpoint
        );

        BufferReader reader(receive_buffer);

        DNSHeader header{
            .id = ntohs(reader.read<uint16_t>()),
            .flags = ntohs(reader.read<uint16_t>()), // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
            .qdcount = ntohs(reader.read<uint16_t>()),
            .ancount = ntohs(reader.read<uint16_t>()),
            .nscount = ntohs(reader.read<uint16_t>()),
            .arcount = ntohs(reader.read<uint16_t>())
        };

        std::vector<DNSQuestion> questions;
        for (uint16_t i = 0; i < header.qdcount; ++i) {
            DNSQuestion q;
            q.qname = parse_domain_name(reader);
            q.qtype = ntohs(reader.read<uint16_t>());
            q.qclass = ntohs(reader.read<uint16_t>());
            questions.push_back(q);
        }

        std::vector<DNSResourceRecord> answers;
        for (uint16_t i = 0; i < header.ancount; ++i) {
            DNSResourceRecord record;
            record.name = parse_domain_name(reader);
            record.type = ntohs(reader.read<uint16_t>());
            record.class_type = ntohs(reader.read<uint16_t>());
            record.ttl = ntohl(reader.read<uint32_t>());
            record.rd_length = ntohs(reader.read<uint16_t>());

            for (uint16_t j = 0; j < record.rd_length; ++j) {
                record.rdata.push_back(reader.read<uint8_t>());
            }
            answers.push_back(record);
        }

        DNSHeader sendHeader{
            .id = 12,
            .flags = htons(0b000000100000000), // rd set to 1
            .qdcount = htons(1),
            .ancount = 0,
            .nscount = 0,
            .arcount = 0
        };

        DNSQuestion sendQuestion{
            .qname = ""
        };


    }

    return 0;
}
