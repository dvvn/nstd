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

template<typename Rng, typename Itr>
concept have_append = requires(Rng & rng, Itr itr)
{
	rng.append(itr, itr);
};

template<typename Rng, typename Itr>
concept have_insert = requires(Rng & rng, Itr itr)
{
	rng.insert(itr, itr);
};

template<typename T, typename A>
void append_proxy(size_t& offset, T& rng, A&& arg)
{
	auto bg = arg.begin( );
	auto ed = arg.end( );

	if (rng.empty( ))
	{
		rng.assign(bg, ed);
		return;
	}

	//constexpr bool rvalue = std::is_rvalue_reference_v<decltype(arg)>;
	using itr_t = decltype(bg);

	if constexpr (have_append<T, itr_t>)
		rng.append(bg, ed);
	else
		rng.insert(rng.begin( ) + offset, bg, ed);

	offset += std::distance(bg, ed);
}

export namespace nstd::inline container
{
	template<typename T, typename ...Args>
	void append(T & cont, Args&& ...args)
	{
		size_t offset = cont.size( );
		if constexpr (sizeof...(Args) > 1)
		{
			const auto buff_size = (args.size( ) + ...);
			cont.resize(offset + buff_size);

			if constexpr (std::random_access_iterator<typename T::iterator>)
			{
				auto itr = cont.begin( ) + offset;
				(copy_or_move(itr, std::forward<Args>(args)), ...);
				return;
			}
		}

		(append_proxy(offset, cont, std::forward<Args>(args)), ...);
	}

	template<typename T, typename ...Args>
	T append(Args&& ...args)
	{
		T out;
		append(out,std::forward<Args>(args)...);
		return out;
	}
}