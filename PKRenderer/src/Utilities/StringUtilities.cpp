#include "PrecompiledHeader.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/Log.h"

namespace PK::Utilities::String
{
	std::string Trim(const std::string& value)
	{
		auto first = value.find_first_not_of(" \n\r");
	
		if (first == std::string::npos)
		{
			return value;
		}
	
		return value.substr(first, (value.find_last_not_of(" \n\r") - first + 1));
	}
	
	std::vector<std::string> Split(const std::string& value, const char* symbols)
	{
		std::vector<std::string> output;
	
		auto start = value.find_first_not_of(symbols, 0);
	
		while (start != std::string::npos)
		{
			auto end = value.find_first_of(symbols, start);
	
			if (end == std::string::npos)
			{
				output.push_back(value.substr(start));
				break;
			}
	
			output.push_back(value.substr(start, end - start));
			start = value.find_first_not_of(symbols, end);
		}
	
		return output;
	}
	
	std::string ReadFileName(const std::string& filepath)
	{
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		return filepath.substr(lastSlash, count);
	}

    std::string ReadDirectory(const std::string& filepath)
    {
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		return filepath.substr(0, lastSlash);
    }

	size_t FirstIndexOf(const char* str, char c)
	{
		auto l = strlen(str);

		for (auto i = 0; i < l; ++i)
		{
			if (str[i] == c)
			{
				return i;
			}
		}

		return std::string::npos;
	}

	size_t LastIndexOf(const char* str, char c)
	{
		auto l = strlen(str);

		for (int i = (int)l - 1; i >= 0; --i)
		{
			if (str[i] == c)
			{
				return i;
			}
		}

		return std::string::npos;
	}
}