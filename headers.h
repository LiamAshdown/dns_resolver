#pragma once
#include <cstdint>
#include <vector>

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
    uint16_t rd_length;
    std::vector<uint8_t> rdata;
};

struct DNSPayload {
    DNSHeader header;
    std::vector<DNSQuestion> questions;
    std::vector<DNSResourceRecord> answers;

    DNSPayload& operator=(const DNSPayload& other) {
        if (this == &other) {
            return *this;
        }

        const uint16_t original_id = this->header.id;

        this->header = other.header;
        this->questions = other.questions;
        this->answers = other.answers;

        this->header.id = original_id;

        return *this;
    }
};

namespace QR {
    constexpr uint16_t QUERY = 0 << 15;
    constexpr uint16_t RESPONSE = 1 << 15;
}

namespace Opcode {
    constexpr uint16_t STANDARD_QUERY = 0 << 11;
    constexpr uint16_t INVERSE_QUERY = 1 << 11;
    constexpr uint16_t STATUS = 2 << 11;
    constexpr uint16_t NOTIFY = 4 << 11;
    constexpr uint16_t UPDATE = 5 << 11;
}

namespace AA {
    constexpr uint16_t QUESTION = 0 << 10;
    constexpr uint16_t AUTHORITATIVE = 1 << 10;
}

namespace TC {
    constexpr uint16_t TC_OFF = 0 << 9;
    constexpr uint16_t TC_ON = 1 << 9;
}

namespace RD {
    constexpr uint16_t RD_OFF = 0 << 8;
    constexpr uint16_t RD_ON = 1 << 8;
}

namespace RA {
    constexpr uint16_t RA_OFF = 0 << 7;
    constexpr uint16_t RA_ON = 1 << 7;
}

namespace Z {
    constexpr uint16_t UNK = 0 << 4;
    constexpr uint16_t ZERO = 0 << 4;
}

namespace RCODE {
    constexpr uint16_t NO_ERROR = 0;
    constexpr uint16_t FORMAT_ERROR = 1;
    constexpr uint16_t SERVER_FAILURE = 2;
    constexpr uint16_t NAME_ERROR = 3;
    constexpr uint16_t NOT_IMPLEMENTED = 4;
    constexpr uint16_t REFUSED = 5;
}