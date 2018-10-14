#pragma once

#include <string>

namespace app_strings_convertion
{
	class strings 
	{
	public:
		static std::string ws2s(const std::wstring& s) noexcept;
	};
}

