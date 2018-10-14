#include "pch.h"
#include "times.h"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace app_times;

std::string times::systime(const std::string& f)
{
	auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	tm tm_v = { 0 };
	localtime_s(&tm_v, &t);
	std::ostringstream s;
	s << std::put_time(&tm_v, f.c_str());
	return s.str();
}