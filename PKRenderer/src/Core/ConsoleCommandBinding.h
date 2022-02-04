#pragma once
#include "PrecompiledHeader.h"
#include "Core/Services/Input.h"

namespace PK::Core
{
	struct TokenConsoleCommand
	{
		const std::string& argument;
		bool isConsumed;
	};

	struct ConsoleCommandBinding
	{
		Services::KeyCode keycode = Services::KeyCode::MOUSE1;
		std::string command;
	};

	typedef std::vector<ConsoleCommandBinding> ConsoleCommandBindList;
}