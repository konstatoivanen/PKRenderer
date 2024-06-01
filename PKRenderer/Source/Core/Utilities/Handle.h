#pragma once
#include <stdint.h>

namespace PK
{
	template<typename T>
	struct Handle
	{
		const T* handle = nullptr;
		Handle(const T* ptr) { handle = ptr; }
		void operator = (const T* ptr) { handle = ptr; }
		operator const T* () const { return handle; }
	};

	template<typename T>
	struct HandleArray
	{
		const T* const* handles;
		uint64_t size;
		HandleArray(const T* const* ptr, uint64_t size = 0ull) : size(size) { handles = ptr; }
		void operator = (const T* const* ptr) { handles = ptr; }
		operator const T* const* () const { return handles; }
	};
}