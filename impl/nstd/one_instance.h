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
		constexpr one_instance& operator=(const one_instance& other) = delete;

		static constexpr size_t instance_index = Index;

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

		static void _Reload( )
		{
			auto ptr = get_ptr( );
			std::destroy_at(ptr);
			std::construct_at(ptr);
		}
	};
}
