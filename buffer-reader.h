#pragma once

#include <stdexcept>
#include <vector>
#include <format>
#include <string>
#include <stdint.h>

class BufferReader {
public:
    BufferReader(const uint32_t size = 512) : _offset(0) {
        _buffer.resize(size);
    }

    template<typename T>
    T read() {
        const std::size_t size = sizeof(T);

        check_size(size);

        T val;
        std::memcpy(&val, _buffer.data() + _offset, size);
        _offset += size;

        if (size == 2) {
            return ntohs(val);
        } else if (size == 4) {
            return static_cast<uint32_t>(ntohl(val));
        }

        return val;
    }

    std::string read_string(const int length) {
        check_size(length);

        std::string val(reinterpret_cast<const char *>(_buffer.data() + _offset), length);

        _offset += length;

        return val;
    }

    std::size_t offset() const {
        return _offset;
    }

    const std::vector<char>& buffer() const {
        return _buffer;
    }

    char* buffer_pointer() {
        return _buffer.data() + _offset;
    }

    void seek(const std::size_t offset) {
        _offset = offset;
    }

    void resize(const std::size_t size) {
        _buffer.resize(size);
    }

    void reset() {
        _offset = 0;
        _buffer.clear();
        _buffer.resize(512);
    }
private:
    void check_size(std::size_t size) {
        if (_offset + size > _buffer.size()) {
            throw std::runtime_error(std::format("Could not read buffer, overflow. Offset: {}, Size: {}, Buffer Size: {}", _offset, size, _buffer.size()));
        }
    }

    std::vector<char> _buffer;
    std::size_t _offset;
};