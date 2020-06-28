#pragma once

// String Extensions
namespace Omnia::Utility::String {

constexpr unsigned int ToHash(const char *str, int h = 0) {
	return !str[h] ? 5381 : (ToHash(str, h + 1) * 33) ^ str[h];
}

inline unsigned int ToHash(const std::string &str) {
	return ToHash(str.c_str());
}

}
