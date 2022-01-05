#pragma once
#include <memory>

namespace nstd
{
	template <typename T, size_t Index = 0>
	class one_instance
	{
	public:
		constexpr one_instance( ) = default;
		constexpr one_instance(const one_instance& other) = delete;
		constexpr one_instance(one_instance&& other) noexcept = default;
		constexpr one_instance& operator=(const one_instance& other) = delete;
		constexpr one_instance& operator=(one_instance&& other) noexcept = default;

		static constexpr size_t index = Index;

		using one_instance_tag = void*;

		/*using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;*/

		using element_type = T;

		static T& get( )
		{
			static_assert(std::is_default_constructible_v<T>, __FUNCSIG__": element must be default constructible!");
			static T cache = {};
			return cache;
		}

		static T* get_ptr( )
		{
			return std::addressof(get( ));
		}
	};

	template <typename T>
	concept is_one_instance = requires
	{
		typename T::one_instance_tag;
		T::index;
		T::get;
	};

	namespace detail
	{
		template <class T>
		void reload_one_instance_impl(T& obj)
		{
			if constexpr (std::swappable<T> && false)
			{
				T replace = {};
				std::swap(obj, replace);
			}
			else
			{
				std::_Destroy_in_place(obj);
				std::_Construct_in_place(obj);
			}
		}
	}

	template <is_one_instance I>
	void reload_one_instance(I& ins)
	{
		detail::reload_one_instance_impl(ins.get( ));
	}

	template <is_one_instance I>
	void reload_one_instance( )
	{
		detail::reload_one_instance_impl(I::get( ));
	}
}
