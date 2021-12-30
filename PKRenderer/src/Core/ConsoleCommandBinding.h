#pragma once
#include "PrecompiledHeader.h"
#include "Core/Services/Input.h"

namespace PK::Core
{
	using namespace Services;

	struct ConsoleCommandToken
	{
		const std::string& argument;
		bool isConsumed;
	};

	struct ConsoleCommandBinding
	{
		KeyCode keycode = KeyCode::MOUSE1;
		std::string command;
	};

	typedef std::vector<ConsoleCommandBinding> ConsoleCommandBindList;
}