#pragma once

#include <random>
#include <xhash>

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

namespace Omnia {

struct UUID_t { unsigned char bytes[37]; }; // 8-4-4-4-12

namespace {

static array<uint8_t, 37> DashPositions = {
    0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

static char *ValidChars = "0123456789abcdef";

}

// Universally Unique Identifier (UUID)
class UUID {
public:
    UUID(): mID { sUniformDistributionInt(sEngine) } {
        for (int i = 0; i < 36; i++) {
            Data.bytes[i] = (DashPositions[i] == 1) ? '-' : ValidChars[(sUniformDistribution(sEngine) % 16)];
        }
        printf("%s\n", Data.bytes);
    }
    UUID(uint64_t uuid): mID { uuid } {}
    UUID(const UUID &other): Data { other.Data }, mID { other.mID } {}
    ~UUID() = default;

    const string GetData() {
        string test;
        test.reserve(37);
        for (int i = 0; i < 36; i++) {
            test += Data.bytes[i];
        }
        return test;
    }

    operator const string() { return GetData(); }

    operator uint64_t() { return mID; }
    operator const uint64_t() const { return mID; }

private:
    UUID_t Data = {0};
    uint64_t mID;

    static inline std::random_device sRandomDevice;
    static inline std::mt19937_64 sEngine { sRandomDevice() };
    static inline std::uniform_int_distribution<unsigned short> sUniformDistribution;
    static inline std::uniform_int_distribution<uint64_t> sUniformDistributionInt;
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
