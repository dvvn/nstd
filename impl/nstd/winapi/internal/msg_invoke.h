#pragma once

#include <concepts>

namespace nstd::winapi
{
	template<typename Msg, typename ...Args>
	void _Invoke_msg(Args&&...args) noexcept
	{
		if constexpr (std::invocable<Msg, decltype(args)...>)
		{
			Msg msg;
			msg(std::forward<Args>(args)...);
		}
	}
}