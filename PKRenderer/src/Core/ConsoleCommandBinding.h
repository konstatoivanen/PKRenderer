#pragma once
#include "PrecompiledHeader.h"
#include "Core/Input.h"

namespace PK::Core
{
	struct ConsoleCommandToken
	{
		const std::string& argument;
		bool isConsumed;
	};

	struct ConsoleCommandBinding
	{
		PK::Core::KeyCode keycode = PK::Core::KeyCode::MOUSE1;
		std::string command;
	};

	class ConsoleCommandBindList : public std::vector<ConsoleCommandBinding>
	{
	};
}