#pragma once

#include <filesystem>
#include <fstream>
#include <string>

/**
* @brief Helper: File System Object Information
*/

/* Retrieve the extension of a given file system object. */
static const std::string GetFileExtension(const std::string &object) noexcept {
	std::filesystem::path result = object;
	return result.extension().string();
}

/* Retrieve the name of a given file system object. */
static const std::string GetFileName(const std::string &object) noexcept {
	std::filesystem::path result = object;
	return result.stem().string();
}

/* Retrieve the path of a given file system object. */
static const std::string GetFilePath(const std::string &object) noexcept {
	std::filesystem::path result = object;
	return result.parent_path().string();
}

/* Retrieve the root directory of a given file system object. */
static const std::string GetFileRoot(const std::string &object) noexcept {
	std::filesystem::path result = object;
	return result.root_path().string();
}

/**
* @brief Helper: File System Object Operations
*/

/* Read data from file system object. */
static std::string ReadFile(const std::string &file) {
	std::string result;
	std::ifstream stream(file, std::ios::in | std::ios::binary);
	if (stream) {
		stream.seekg(0, std::ios::end);
		size_t size = stream.tellg();
		if (size != -1) {
			result.resize(size);
			stream.seekg(0, std::ios::beg);
			stream.read(&result[0], size);
			stream.close();
		} else {
			// Error while reading file
		}
	} else {
		// Error while open file
	}
	return result;
}

/* Test: Read data from file system object. */
static std::string ReadFile2(const std::string &object) {
	std::ifstream stream(object, std::ios::binary|std::ios::ate|std::ios::in);
	if (!stream) throw std::runtime_error(object + ": " + std::strerror(errno));


	std::string result = "";
	//if (stream) {
	//	std::stringstream FileCache;

	//	FileStream.open(object);
	//	FileCache << FileStream.rdbuf();
	//	FileStream.close();


	//} else {
	//	applog << Log::Error << "The specified file '" << object << "' doesn't exist!\n";
	//}


	//


	//std::string result;
	//std::ifstream stream(file, std::ios::in | std::ios::binary);
	//if (stream) {
	//	stream.seekg(0, std::ios::end);
	//	size_t size = stream.tellg();
	//	if (size != -1) {
	//		result.resize(size);
	//		stream.seekg(0, std::ios::beg);
	//		stream.read(&result[0], size);
	//		stream.close();
	//	} else {
	//		// Error while reading file
	//	}
	//} else {
	//	// Error while open file
	//}
	return result;
}