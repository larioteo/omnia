#pragma once

#include "Omnia/Omnia.pch"

namespace Omnia {

/**
* @brief Timestep which holds a timestamp in milliseconds.
*/
class Timestamp {
	// Properties
	double Time;

public:
	// Default
	
	
	Timestamp(double time = 0.0f): Time{ time } {}
	~Timestamp() = default;

	/* Retrive the timestamp in seconds */
	double GetSeconds()			{ return Time / 1000.0f; }

	/* Retrive timestamp in milliseconds */
	uint64_t GetMilliseconds()	{ return Time; }

	// Operators
	operator double() { return GetSeconds(); }
};


/**
 * @brief Timer that counts ticks until GetDeltaTime or Reset is called.
*/
class Timer {
	// Properties
	using Clock = std::chrono::high_resolution_clock;
	using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
	using Timespan = std::chrono::duration<double>;
	Timestamp StartTime;

public:
	enum class Unit: uint8_t {
		Seconds,
		Milliseconds,
		Microseconds,
		Nanoseconds,
	};
	
	// Default
	Timer() { Reset(); }
	~Timer() = default;

	/** Retrive current clock tick */
	const uint64_t Now() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(CalculateDuration()).count();
	}

	/** Retrive delta time in milliseconds (default) */
	const uint64_t GetDeltaTime() {
		uint64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(CalculateDuration()).count();
		Reset();
		return duration;
	}

	/** Retrive delta time in specified period (s, ms, µs, ns) */
	const uint64_t GetDeltaTimeAs(Unit unit = Unit::Seconds) {
		uint64_t duration = 0;
		switch (unit) {
			case Unit::Seconds:			{ duration = std::chrono::duration_cast<std::chrono::seconds>(CalculateDuration()).count();			break; }
			case Unit::Milliseconds:	{ duration = std::chrono::duration_cast<std::chrono::milliseconds>(CalculateDuration()).count();	break; }
			case Unit::Microseconds:	{ duration = std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count();	break; }
			case Unit::Nanoseconds:		{ duration = std::chrono::duration_cast<std::chrono::nanoseconds>(CalculateDuration()).count();		break; }
			default:					{ break; }
		}
		Reset();
		return duration;
	}

	/** Reset timer manually (automatically done by GetDeltaTimeX) */
	void Reset() { StartTime = Clock::now(); }

private:
	Timespan CalculateDuration() {
		return { Clock::now() - StartTime };
	}
};

}
