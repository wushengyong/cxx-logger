#pragma once

#include <string>

namespace app_times
{

	class times
	{
	public:
		static std::string systime(const std::string& f = "%Y-%m-%d %H:%M:%S");
	};

}