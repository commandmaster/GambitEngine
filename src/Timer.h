#pragma once

#include <chrono>
#include <iostream>

class Timer
{
public:
	Timer();

	void start();

	void stop();
	
	template<typename Duration = std::chrono::milliseconds>
	double elapsedTime() const;

private:
	std::chrono::high_resolution_clock::time_point startPoint;
	std::chrono::high_resolution_clock::time_point endPoint;
};

template<typename Duration>
double Timer::elapsedTime() const
{
	auto timeSpan = std::chrono::duration_cast<Duration>(endPoint - startPoint);
	return static_cast<double>(timeSpan.count());
}
