#pragma once

#include <random>

class Random {
    static std::mt19937 sRandomEngine;
    static std::uniform_int_distribution<std::mt19937::result_type> sRandomsDistribution;
public:
    static void Init() {
        sRandomEngine.seed(std::random_device()());
    }

    static float Float() {
        return (float)sRandomsDistribution(sRandomEngine) / (float)std::numeric_limits<uint32_t>::max();
    }
};

std::mt19937 Random::sRandomEngine;
std::uniform_int_distribution<std::mt19937::result_type> Random::sRandomsDistribution;
