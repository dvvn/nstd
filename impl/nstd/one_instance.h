#pragma once
#include <memory>

namespace nstd
{
	template <typename T, size_t Index = 0>
	class one_instance
	{
	public:
		constexpr one_instance( )                                        = default;
		constexpr one_instance(const one_instance& other)                = delete;
		constexpr one_instance(one_instance&& other) noexcept            = default;
		constexpr one_instance& operator=(const one_instance& other)     = delete;
		constexpr one_instance& operator=(one_instance&& other) noexcept = default;

		static constexpr size_t index = Index;

		using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;

		static reference get( )
		{
			static_assert(std::is_default_constructible_v<T>, __FUNCSIG__": element must be default constructible!");
			static T cache = T( );
			return cache;
		}

		static pointer get_ptr( )
		{
			return std::addressof(get( ));
		}
	};

	template <typename T>
	concept is_one_instance = requires
	{
		typename T::value_type;
		T::index;
	} && std::derived_from<T, one_instance<typename T::value_type, T::index>>;

	template <is_one_instance I>
	void reload_one_instance(I& ins)
	{
		auto& ref = ins.get( );
		std::_Destroy_in_place(ref);
		std::_Construct_in_place(ref);
	}
}
