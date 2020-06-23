#pragma once

#include "Omnia/Omnia.pch"

namespace Omnia {

/**
* @brief Timestamp which holds a double in milliseconds.
*/
class Timestamp {
	// Properties
	double Time;

public:
	// Default
	Timestamp(double time = 0.0): Time{ time } {}
	~Timestamp() = default;

	/* Retrive the timestamp in seconds */
	const double GetSeconds() { return Time / 1000.0; }

	/* Retrive timestamp in milliseconds */
	const double GetMilliseconds() { return Time; }

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
	const double Now() {	return std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count() / 1000.0; }

	/** Retrive delta time in milliseconds (default) */
	const double GetDeltaTime() {
		double duration = std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count() / 1000.0;
		Reset();
		return duration;
	}

	/** Retrive delta time in specified period (s, ms, µs, ns) */
	const double GetDeltaTimeAs(Unit unit = Unit::Seconds) {
		double duration = 0.0;
		switch (unit) {
			case Unit::Seconds:			{ duration = std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count() / 1000000.0;	break; }
			case Unit::Milliseconds:	{ duration = std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count() / 1000.0;		break; }
			case Unit::Microseconds:	{ duration = std::chrono::duration_cast<std::chrono::microseconds>(CalculateDuration()).count();				break; }
			case Unit::Nanoseconds:		{ duration = std::chrono::duration_cast<std::chrono::nanoseconds>(CalculateDuration()).count();					break; }
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
