#pragma once

#include <vector>
#include <winsock2.h>

class BufferWriter {
public:
    BufferWriter() {}

    template<typename T>
    void write(T val) {
        const std::size_t size = sizeof(T);

        if (size == 2) {
            val = htons(val);
        } else if (val == 4) {
            val = htonl(val);
        }

        write_bytes(reinterpret_cast<const uint8_t>(&val), sizeof(T));
    }

    void write_bytes(const uint8_t* src, const std::size_t length) {
        if (length == 0) {
            return;
        }

        _buffer.insert(_buffer.end(), src, src + length);
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
