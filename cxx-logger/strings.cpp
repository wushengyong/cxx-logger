#include "pch.h"
#include "strings.h"
#include <vector>

using namespace app_strings_convertion;

std::string strings::ws2s(const std::wstring&s) noexcept
{
	std::locale sys_local("");

	const wchar_t* data_from = s.c_str();
	const wchar_t* data_from_end = s.c_str() + s.size();
	const wchar_t* data_from_next = 0;

	int wchar_size = 4;
	std::vector<char> ds;
	ds.resize((s.size() + 1) * wchar_size);

	char* data_to = &ds[0];
	char* data_to_end = data_to + ds.size();
	char* data_to_next = 0;

	typedef std::codecvt<wchar_t, char, mbstate_t> convert_facet;

	mbstate_t out_state = { 0 };
	auto result = std::use_facet<convert_facet>(sys_local).out(
		out_state, data_from, data_from_end, data_from_next,
		data_to, data_to_end, data_to_next
	);
	if (result == convert_facet::ok) {
		return &ds[0];
	}
	return std::string();
}