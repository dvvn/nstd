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
concept have_assign = requires(Rng & rng, Itr itr)
{
	rng.assign(itr, itr);
};

template<typename Rng, typename Itr>
concept have_append = requires(Rng & rng, Itr itr)
{
	rng.append(itr, itr);
};

template<typename Rng, typename Itr>
concept have_hint = requires(Rng & rng, Itr itr)
{
	rng.insert(rng.begin( ), itr, itr);
};

template<typename Rng, typename Itr>
concept have_insert_rng = requires(Rng & rng, Itr itr)
{
	rng.insert(itr, itr);
};

template<typename Rng>
concept have_reserve = requires(Rng & rng)
{
	rng.reserve(1337);
};

template<typename T, typename A>
void append_proxy(size_t& offset, T& dst, A&& src)
{
	const auto bg = src.begin( );
	const auto ed = src.end( );

	//constexpr bool rvalue = std::is_rvalue_reference_v<decltype(src)>;
	using itr_t = decltype(bg);

	auto pos = std::next(dst.begin( ), offset);

	if constexpr (have_hint<T, itr_t>)
	{
		dst.insert(pos, bg, ed);
	}
	else
	{
		using val_t = typename T::value_type;
		if (pos == dst.end( ))
		{
			for (auto itr = bg; itr != ed; ++itr)
				dst.insert(dst.end( ), val_t(*itr));
		}
		else
		{
			for (auto itr = bg; itr != ed; ++itr)
				pos = dst.insert(pos, val_t(*itr));
		}
	}

	offset += std::distance(bg, ed);
}

export namespace nstd::inline container
{
	template<typename T, typename ...Args>
	void append(T & cont, Args&& ...args)
	{
		constexpr auto args_count = sizeof...(Args);
		auto offset = cont.size( );
		if constexpr (args_count > 1 && std::random_access_iterator<typename T::iterator>)
		{
			const auto buff_size = (args.size( ) + ...);
			cont.resize(offset + buff_size);

			auto itr = cont.begin( ) + offset;
			(copy_or_move(itr, std::forward<Args>(args)), ...);

		}
		else
		{
			if constexpr (args_count > 1 && have_reserve<T>)
			{
				const auto buff_size = (args.size( ) + ...);
				cont.reserve(offset + buff_size);
			}

			(append_proxy(offset, cont, std::forward<Args>(args)), ...);
		}
	}

	template<typename T, typename ...Args>
	T append(Args&& ...args)
	{
		T out;
		append(out, std::forward<Args>(args)...);
		return out;
	}
}