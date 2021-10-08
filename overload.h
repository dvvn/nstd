#pragma once
#include <type_traits>

namespace nstd
{
	template <class ...Fs>
	struct overload: Fs...
	{
		template <class ...Ts>
		constexpr overload(Ts&& ...ts) : Fs{std::forward<Ts>(ts)}...
		{
		}

		using Fs::operator()...;
	};

	template <class ...Ts>
	overload(Ts&&...) -> overload<std::remove_reference_t<Ts>...>;
}
