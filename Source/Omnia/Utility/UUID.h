#pragma once

/* 
 * Universally Unique Identifier (UUID)
 * Source:    https://tools.ietf.org/html/rfc4122
 * 
 * [ Version |  Description             ]
 * --------------------------------------
 * [    1    |  timestamp based         ]
 * [    2    |  DCE security version    ]
 * [    3    |  name-based (MD5)        ]
 * [    4    |  random generated        ]
 * [    5    |  name-based (SHA-1)      ]
 * --------------------------------------
 * 
 * uint128_t    = uint32_t - uint16_t  - uint16_t  - (uint8_t + uint8_t)   - uint48_t
 * 32           = 8        - 4         - 4         - 4                     - 12
*/

#include <random>

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

namespace Omnia {

// UUID: Random generated (v4)
class UUID {
public:
    UUID() {
        mIDC.reserve(36);
        mIDS.reserve(36);
        Generate();
    }
    UUID(const UUID &other): mID { other.mID }, mIDS { other.mIDS }, mIDC { other.mIDC } {}
    UUID(const string &uuid): mIDS { uuid } {}
    ~UUID() = default;

    const uint64_t GetData() {
        return mID;
    }
    const string &GetData1() {
        return mIDC;
    }
    const string &GetData2() {
        return mIDS;
    }

    void operator=(string right) {
        mIDS = right;
    }
    void operator=(uint64_t right) {
        mID = right;
    }

    bool operator==(string right) {
        return (mIDS == right);
    }
    bool operator==(uint64_t right) {
        return (mID == right);
    }

    operator string() { return GetData2(); }
    operator const string() { return GetData2(); }

    operator uint64_t() { return mID; }
    operator const uint64_t() const { return mID; }

private:
    void Generate() {
        GenerateMethod0();
        GenerateMethod1();
        GenerateMethod2();
    }
    void GenerateMethod0() {
        mID = sUniformDistribution(sEngine);
    }
    void GenerateMethod1() {
        static const char *ValidChars = "0123456789abcdef";
        static array<uint8_t, 36> DashPositions = {
            0, 0, 0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 1,
            0, 0, 0, 0, 1,
            0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        for (int i = 0; i < 36; i++) {
            mIDC += (DashPositions[i] == 1) ? '-' : ValidChars[(sUniformDistributionShort(sEngine) % 16)];
        }
    }
    void GenerateMethod2() {
        stringstream result;
        result << std::hex;
        for (int i = 0; i < 8; i++) result << sUniformDistribution(sEngine, 15);
        result << "-";
        for (int i = 0; i < 4; i++) result << sUniformDistribution(sEngine, 15);
        result << "-";
        for (int i = 0; i < 4; i++) result << sUniformDistribution(sEngine, 15);
        result << "-";
        for (int i = 0; i < 4; i++) result << sUniformDistribution(sEngine, 15);
        result << "-";
        for (int i = 0; i < 12; i++) result << sUniformDistribution(sEngine, 15);
        mIDS= result.str();
    }

private:
    uint64_t mID = 0;
    string mIDC = ""s;
    string mIDS = ""s;

    static inline std::random_device sRandomDevice;
    static inline std::mt19937_64 sEngine { sRandomDevice() };
    static inline std::uniform_int_distribution<uint64_t> sUniformDistribution;
    static inline std::uniform_int_distribution<unsigned short> sUniformDistributionShort;
};

}
