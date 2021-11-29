#pragma once
#include "PrecompiledHeader.h"

namespace PK::Utilities::String
{
	std::string Trim(const std::string& value);
	std::vector<std::string> Split(const std::string& value, const char* symbols);
	std::string ReadFileName(const std::string& filepath);
	std::string ReadDirectory(const std::string& filepath);
	size_t FirstIndexOf(const char* str, char c);
	size_t LastIndexOf(const char* str, char c);
};

