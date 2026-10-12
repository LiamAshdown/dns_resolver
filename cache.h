#pragma once

#include "headers.h"

#include <unordered_map>

struct CacheKey {
    std::string name;
    uint16_t type;
    uint16_t cls;

    bool operator==(const CacheKey& key) const = default;
};

struct CacheKeyHash {
    std::size_t operator()(const CacheKey& key) const {
        return std::hash<std::string>{}(key.name) ^ (key.type << 16) ^ key.cls;
    }
};

struct CacheEntry {
    DNSPayload payload;
    std::chrono::steady_clock::time_point stored;
    std::chrono::steady_clock::time_point expires;

};

class Cache {
public:
    static CacheKey make_key(const DNSQuestion& question) {
        std::string name = question.qname;
        std::ranges::transform(name, name.begin(), ::tolower);

        return CacheKey({name, question.qtype, question.qclass});
    }

    void add_key(const CacheKey &key, const DNSPayload& dns) {
        const uint32_t min_ttl = std::ranges::min(dns.answers, {}, &DNSResourceRecord::ttl).ttl;
        const auto now = std::chrono::steady_clock::now();

        _cache[key] = {dns, now, now + std::chrono::seconds(min_ttl)};
    }

    bool has_key(const CacheKey& key) const {
        return _cache.contains(key);
    }

    CacheEntry *get_cache(const CacheKey &key) {
        const auto it = _cache.find(key);
        if (it != _cache.end()) {
            return &it->second;
        }

        return nullptr;
    }


    void remove_cache(const CacheKey& key) {
        _cache.erase(key);
    }

private:
    std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> _cache;
};
