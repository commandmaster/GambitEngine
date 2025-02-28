#include "Timer.h"

Timer::Timer()
{
	startPoint = std::chrono::high_resolution_clock::now();
	endPoint = std::chrono::high_resolution_clock::now();
}

void Timer::start()
{
	startPoint = std::chrono::high_resolution_clock::now();
	endPoint = std::chrono::high_resolution_clock::now();
}

void Timer::stop()
{
	endPoint = std::chrono::high_resolution_clock::now();
}
