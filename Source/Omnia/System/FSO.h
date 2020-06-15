#pragma once

#include <fstream>
#include <string>

static std::string ReadFile(const std::string &file) {
	std::string result;
	std::ifstream in(file, std::ios::in | std::ios::binary);
	if (in) {
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();
		if (size != -1) {
			result.resize(size);
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
			in.close();
		} else {
			// Error while reading file
		}
	} else {
		// Error while open file
	}
	return result;
}
