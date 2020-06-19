#pragma once

#include <chrono>

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"

namespace Omnia {

using namespace std::literals::chrono_literals;

class Timestep {
	float Time;

public:
	Timestep(float time = 0.0f): Time{ time } {}
	~Timestep() = default;

	operator float() { return Time / 1000.0f; }

	float GetSeconds() { return Time / 1000.0f; }
	float GetMiliseconds() { return Time; }
	float GetMicroseconds() { return Time * 1000.0f; }
};


class Timer {
	using Clock = std::chrono::high_resolution_clock;
	using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
	using Timespan = std::chrono::duration<float>;
	using Nanoseconds = std::chrono::nanoseconds;

	Timespan Duration;
	Timestamp StartTime;

public:
	Timer() { StartTime = Clock::now(); }
	~Timer() { GetDeltaTime(); }

	static Timestamp Now() {
		return Clock::now();
	}

	float GetDeltaTime() {
		Duration = Clock::now() - StartTime;
		StartTime = Clock::now();
		return (std::chrono::duration_cast<Nanoseconds>(Duration)).count() * 0.000001;
	}

	void SetStartTime() {
		StartTime = Clock::now();
	}
};

}

//std::chrono::high_resolution_clock::time_point last = clock::now();
//std::chrono::high_resolution_clock::time_point now = clock::now();
//std::chrono::duration<double> delta = 0s;
//std::chrono::nanoseconds lag(0ms);
//double alpha = 0.0;
//unsigned long long frames = 0;
