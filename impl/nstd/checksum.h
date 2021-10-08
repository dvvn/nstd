#pragma once
#include <span>
#include <string_view>

namespace std::filesystem
{
	class path;
}

namespace nstd
{
	namespace detail
	{
		struct checksum_impl
		{
			template <typename E, typename Tr>
			size_t operator ()(const std::basic_string_view<E, Tr>& str) const
			{
				return std::_Hash_array_representation(str._Unchecked_begin( ), str.size( ));
			}

			template <class T>
			size_t operator ()(const T& obj) const
				requires(std::invocable<checksum_impl, decltype(obj.view( ))>)
			{
				return std::invoke(*this, obj.view( ));
			}

			template <typename T>
				requires(std::is_trivially_destructible_v<T>)
			size_t operator ()(const std::span<T>& vec) const
			{
				return std::_Hash_array_representation(vec._Unchecked_begin( ), vec.size( ));
			}

			size_t operator()(const std::filesystem::path& p) const;
		};
	}

	inline constexpr auto checksum = detail::checksum_impl( );
}
