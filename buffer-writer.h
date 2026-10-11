#pragma once

#include <vector>

class BufferWriter {
public:
    BufferWriter(const uint32_t size = 512) {
        _buffer.reserve(512);
    }

    template<typename T>
    void write(T val) {
        const std::size_t size = sizeof(T);

        if (size == 2) {
            val = htons(val);
        } else if (size == 4) {
            val = htonl(val);
        }

        write_bytes(reinterpret_cast<const char*>(&val), sizeof(T));
    }

    void write_bytes(const char* src, const std::size_t length) {
        if (length == 0) {
            return;
        }

        _buffer.insert(_buffer.end(), src, src + length);
    }

    void write_string(const std::string &str) {
        write_bytes(reinterpret_cast<const char *>(reinterpret_cast<const uint8_t*>(str.data())), str.size());
    }

    std::size_t size() const {
        return _buffer.size();
    }

    const uint8_t* data() const {
        return _buffer.data();
    }
private:
    std::vector<uint8_t> _buffer{};
};
