#pragma once
#include <memory>

namespace nstd
{
	template <typename T, size_t Index = 0>
	class one_instance
	{
	public:
		constexpr one_instance( ) = default;

		using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;

		/*static reference get( )
		{
			static_assert(std::is_default_constructible_v<T>, "T must be default constructible!");
			static T cache = T( );
			return cache;
		}*/

		static pointer get_ptr( )
		{
			static_assert(std::is_default_constructible_v<T>, __FUNCSIG__": element must be default constructible!");
			static T cache = T( );
			return std::addressof(cache);
		}
	};

	template <typename T, size_t Index = 0>
	class one_instance_shared
	{
	public:
		using value_type = T;
		using reference = T&;
		using const_reference = const T&;
		using pointer = T*;
		using const_pointer = const T*;
		using weak_type = std::weak_ptr<T>;
		using shared_type = std::shared_ptr<T>;

		static shared_type get_ptr_shared(bool steal = false)
		{
			static_assert(std::is_default_constructible_v<T>, __FUNCTION__": T must be default constructible!");

			static shared_type shared = std::make_shared<T>( );
			static weak_type   weak;

			if (!shared)
			{
				//runtime_assert(steal == false);
				return weak.lock( );
			}
			if (!steal)
				return shared;
			weak = shared;
			return std::move(shared);
		}

		static T* get_ptr( )
		{
			static T* holder = []
			{
				return get_ptr_shared(false).get( );
			}( );

			return holder;
		}

		static weak_type get_ptr_weak( )
		{
			static weak_type weak = get_ptr_shared(false);
			return weak;
		}
	};

	template <typename T>
	concept is_one_instance = requires
	{
		{ one_instance(T( )) }->std::destructible;
	};

	template <typename T>
	concept is_one_instance_shared = requires
	{
		{ one_instance_shared(T( )) }->std::destructible;
	};
}
