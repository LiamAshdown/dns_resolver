#pragma once

#include <string>
#include <vector>

#include "buffer-reader.h"

#define MAX_LABEL_SIZE 63
#define MAX_NAMES 255
#define MAX_PACKET_SIZE 512

struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

struct DNSQuestion {
    std::string qname;
    uint16_t qtype;
    uint16_t qclass;
};

struct DNSResourceRecord {
    std::string name;
    uint16_t type;
    uint16_t class_type;
    uint32_t ttl;
    int16_t rd_length;
    std::vector<uint8_t> rdata;
};

class DNS {
public:
    void parse(BufferReader& buffer_reader) {

        DNSHeader header{
            .id = buffer_reader.read<uint16_t>(),
            .flags = buffer_reader.read<uint16_t>(),
            .qdcount = buffer_reader.read<uint16_t>(),
            .ancount = buffer_reader.read<uint16_t>(),
            .nscount = buffer_reader.read<uint16_t>(),
            .arcount = buffer_reader.read<uint16_t>()
        };
    }
};