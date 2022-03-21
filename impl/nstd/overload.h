#pragma once
#include <functional>

namespace nstd
{
	template <class ...Fs>
	struct overload : Fs...
	{
		template <class ...Ts>
		constexpr overload(Ts&& ...ts) : Fs{std::forward<Ts>(ts)}...
		{
		}

		using Fs::operator()...;
	};

	template<typename T>
	constexpr auto overload_function(const T obj)
	{
		if constexpr (std::is_class_v<T>)
			return obj;
		else
			return std::bind_front(obj);
	}

	template <class ...Ts>
	overload(Ts&&...obj)->overload<decltype(overload_function(obj))...>;
}
