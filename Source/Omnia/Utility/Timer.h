#pragma once

#include <chrono>

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"

namespace Omnia {

using namespace std::literals::chrono_literals;

class Timer {
	using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
	using TimeSpan = std::chrono::duration<float>;
	using Clock = std::chrono::high_resolution_clock;

	TimeSpan Duration;
	TimeStamp StartTime;
	TimeStamp StopTime;

public:
	Timer() {
		StartTime = Clock::now();
	}

	~Timer() {
		StopTime = Clock::now();
		Duration = StopTime - StartTime;
	}
};

}
