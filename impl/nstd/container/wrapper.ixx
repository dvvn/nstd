module;

#include <algorithm>

export module nstd.container.wrapper;

template<typename T, typename A>
void copy_or_move(T& itr, A&& arg)
{
	if constexpr (std::is_rvalue_reference_v<decltype(arg)>)
		std::move(arg.begin( ), arg.end( ), itr);
	else
		std::copy(arg.begin( ), arg.end( ), itr);

	itr += arg.size( );
}

template<typename T, typename A>
void append_ex(T& rng, A&& arg)
{
	auto bg = rng.begin( );
	auto ed = rng.end( );
	if constexpr (std::is_rvalue_reference_v<decltype(arg)>)
		rng.append(std::make_move_iterator(bg), std::make_move_iterator(ed));
	else
		rng.append(bg, ed);
}

export namespace nstd::inline container
{
	template<typename T, typename ...Args>
	void append(T & cont, Args&& ...args)
	{
		if constexpr (sizeof...(Args) > 1)
		{
			const auto size_before = cont.size( );
			const auto buff_size = (args.size( ) + ...);
			cont.resize(size_before + buff_size);

			if constexpr (std::random_access_iterator<typename T::iterator>)
			{
				auto itr = cont.begin( ) + size_before;
				(copy_or_move(itr, std::forward<Args>(args)), ...);
				return;
			}
		}

		(append_ex(cont, std::forward<Args>(args)), ...);
	}

	template<typename T, typename ...Args>
	T append(Args&& ...args)
	{
		T out;
		append(out,std::forward<Args>(args)...);
		return out;
	}
}