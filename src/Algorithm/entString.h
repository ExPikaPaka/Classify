#pragma once
#include <string>

namespace ent {
	namespace algorithm {

		std::wstring stringToWstring(const std::string& str) {
			std::wstring wstr(str.begin(), str.end()); // Simple conversion
			return wstr;
		}

		std::string wstringToString(const std::wstring& wstr) {
			return std::string(wstr.begin(), wstr.end());
		}
	}
}