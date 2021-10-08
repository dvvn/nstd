#pragma once

#include <memory>

// ReSharper disable once CppInconsistentNaming
using HANDLE = void*;

namespace nstd::os
{
	struct handle_closer final
	{
		using pointer = HANDLE;
		void operator()(HANDLE ptr) const;
	};

	using handle_base = std::unique_ptr<HANDLE, handle_closer>;

	struct handle: handle_base
	{
		handle( ) = default;
		handle(HANDLE ptr);
	};
}
