#pragma once

#include <string>
#include <vector>

#include "buffer-reader.h"
#include "buffer-writer.h"
#include "headers.h"

#define MAX_LABEL_SIZE 63
#define MAX_NAMES 255
#define MAX_PACKET_SIZE 512


namespace DNS {
    std::string parse_name(BufferReader& buffer_reader) {
        std::string name;

        bool jumped = false;
        std::size_t original_offset = 0;

        while (true) {
            const uint8_t length_byte = buffer_reader.read<uint8_t>();

            if (length_byte == 0) {
                break;
            }

            // First two bits are the compression flag
            if ((length_byte & 0b11000000) == 0b11000000) {
                auto second_byte = buffer_reader.read<uint8_t>();
                // merge last 6 bits and second byte bits to get the offset
                const uint16_t pointer_bytes = ((length_byte & 0b00111111) << 8) | second_byte;

                if (!jumped) {
                    original_offset = buffer_reader.offset();
                    jumped = true;
                }

                buffer_reader.seek(pointer_bytes);
                continue;
            }

            if (!name.empty()) {
                name += ".";
            }

            name += buffer_reader.read_string(length_byte);
        }

        if (jumped) {
            buffer_reader.seek(original_offset);
        }

        return name;
    }

    void encode_name(std::string name, BufferWriter& buffer_writer) {
        std::string encoding = "";
        for (auto token : std::views::split(name, '.')) {
            buffer_writer.write<uint8_t>(token.size());
            buffer_writer.write_string(std::string(token.begin(), token.end()));
        }

        // null terminator
        buffer_writer.write<uint8_t>(0);
    }

    inline DNSPayload parse(BufferReader& buffer_reader) {
        buffer_reader.seek(0);

        const DNSHeader header{
            .id = buffer_reader.read<uint16_t>(),
            .flags = buffer_reader.read<uint16_t>(),
            .qdcount = buffer_reader.read<uint16_t>(),
            .ancount = buffer_reader.read<uint16_t>(),
            .nscount = buffer_reader.read<uint16_t>(),
            .arcount = buffer_reader.read<uint16_t>()
        };

        std::vector<DNSQuestion> questions;
        for (uint16_t i = 0; i < header.qdcount; i++) {
            questions.push_back(DNSQuestion{
                .qname = parse_name(buffer_reader),
                .qtype = buffer_reader.read<uint16_t>(),
                .qclass = buffer_reader.read<uint16_t>()
            });
        }

        std::vector<DNSResourceRecord> answers;
        for (uint16_t i = 0; i < header.ancount; i++) {
            DNSResourceRecord record{
                .name = parse_name(buffer_reader),
                .type = buffer_reader.read<uint16_t>(),
                .class_type = buffer_reader.read<uint16_t>(),
                .ttl = buffer_reader.read<uint32_t>(),
                .rd_length = buffer_reader.read<uint16_t>()
            };

            for (uint16_t j = 0; j < record.rd_length; j++) {
                record.rdata.push_back(buffer_reader.read<uint8_t>());
            }

            answers.push_back(record);
        }

        return DNSPayload({
            .header = header,
            .questions = questions,
            .answers = answers
        });
    }

    inline BufferWriter build(DNSPayload send_dns) {
        BufferWriter buffer_writer;

        buffer_writer.write<uint16_t>(send_dns.header.id);
        buffer_writer.write<uint16_t>(send_dns.header.flags);
        buffer_writer.write<uint16_t>(send_dns.header.qdcount);
        buffer_writer.write<uint16_t>(send_dns.header.ancount);
        buffer_writer.write<uint16_t>(send_dns.header.nscount);
        buffer_writer.write<uint16_t>(send_dns.header.arcount);

        for (uint16_t i = 0; i < send_dns.header.qdcount; i++) {
            DNSQuestion question = send_dns.questions[i];

            encode_name(question.qname, buffer_writer);
            buffer_writer.write<uint16_t>(question.qtype);
            buffer_writer.write<uint16_t>(question.qclass);
        }

        for (const DNSResourceRecord &record : send_dns.answers) {
            encode_name(record.name, buffer_writer);
            buffer_writer.write<uint16_t>(record.type);
            buffer_writer.write<uint16_t>(record.class_type);
            buffer_writer.write<uint32_t>(record.ttl);
            buffer_writer.write<uint16_t>(record.rd_length);
            buffer_writer.write_bytes(reinterpret_cast<const char *>(record.rdata.data()), record.rdata.size());
        }

        return buffer_writer;
    }


};