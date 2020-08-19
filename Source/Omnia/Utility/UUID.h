#pragma once

#include <random>
#include <xhash>

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

namespace Omnia {
// 128-Bit Universally Unique Identifier (UUID)

class UUID {
public:
    UUID(): mID { sUniformDistribution(eng) } {}
    UUID(uint32_t uuid): mID { uuid } {}
    UUID(const UUID &other): mID { other.mID } {}
    ~UUID() = default;

    operator uint64_t() { return mID; }
    operator const uint64_t() const { return mID; }

private:
    uint64_t mID;

    static inline std::random_device sRandomDevice;
    static inline std::mt19937_64 eng { sRandomDevice() };
    static inline std::uniform_int_distribution<uint64_t> sUniformDistribution;
};

}

namespace std {

template <>
struct hash<Omnia::UUID> {
    std::size_t operator()(const Omnia::UUID& uuid) const {
        return hash<uint64_t>()((uint64_t)uuid);
    }
};

}
