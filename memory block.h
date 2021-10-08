#pragma once

#include "address.h"

#include <algorithm>
#include <optional>
#include <ranges>
#include <span>
#include <variant>
#include <vector>
//#include "signature.h"

#define NSTD_MEM_BLOCK_UNWRAP_UNKNOWN_BYTES

namespace nstd
{
	template <typename>
	constexpr bool is_std_optional_v = false;

	template <typename T>
	constexpr bool is_std_optional_v<std::optional<T>> = true;
	template <typename T>
	constexpr bool is_std_optional_v<const std::optional<T>&> = true;
	template <typename T>
	constexpr bool is_std_optional_v<std::optional<T>&> = true;
	template <typename T>
	constexpr bool is_std_optional_v<std::optional<T>&&> = true;

	using known_byte = uint8_t;
	using unknown_byte = std::optional<uint8_t>;

	using known_bytes_range = std::span<known_byte>;
	using known_bytes_range_const = std::span<const known_byte>;
	using unknown_bytes_range = std::span<unknown_byte>;
	using unknown_bytes_range_const = std::span<const unknown_byte>;

	using known_bytes_object = std::vector<known_byte>;
	using unknown_bytes_object = std::vector<unknown_byte>;

	template <std::ranges::random_access_range T>
		requires(std::same_as<std::ranges::range_value_t<T>, unknown_byte>)
	constexpr bool all_bytes_known(const T& rng)
	{
		return std::ranges::all_of(rng, [](const unknown_byte& b) { return b.has_value( ); });
	}

	template <std::ranges::random_access_range T>
		requires(std::same_as<std::ranges::range_value_t<T>, unknown_byte>)
	// ReSharper disable once CppRedundantInlineSpecifier
	_CONSTEXPR20_CONTAINER known_bytes_object make_bytes_known(const T& rng)
	{
		//runtime_assert(all_bytes_known(rng)==true);
		auto tmp = std::ranges::views::transform(rng, [](const unknown_byte& b) { return *b; });
		return known_bytes_object(tmp.begin( ), tmp.end( ));
	}

	class any_bytes_range
	{
		// ReSharper disable CppRedundantInlineSpecifier

	public:
		using value_type = std::variant<known_bytes_object, unknown_bytes_object, known_bytes_range_const, unknown_bytes_range_const>;

		_CONSTEXPR20_CONTAINER explicit any_bytes_range(known_bytes_object&& obj)
			: data_(
					std::in_place_type<known_bytes_object>, std::move(obj)
				   )
		{
		}

		_CONSTEXPR20_CONTAINER explicit any_bytes_range(unknown_bytes_object&& obj)
			: data_(
#ifdef NSTD_MEM_BLOCK_UNWRAP_UNKNOWN_BYTES
					std::in_place_type<unknown_bytes_object>, std::move(obj)
#else
																						   all_bytes_known(obj)
																							   ? value_type(std::in_place_type<known_bytes_object>, make_bytes_known(obj))
																							   : value_type(std::in_place_type<unknown_bytes_object>, std::move(obj))
#endif
				   )
		{
		}

		_CONSTEXPR20 explicit any_bytes_range(const known_bytes_range_const& obj)
			: data_(std::in_place_type<known_bytes_range_const>, (obj))
		{
		}

		_CONSTEXPR20 explicit any_bytes_range(const unknown_bytes_range_const& obj)
			: data_(std::in_place_type<unknown_bytes_range_const>, (obj))
		{
		}

		bool known( ) const;

		known_bytes_range_const   get_known( ) const;
		unknown_bytes_range_const get_unknown( ) const;

	private:
		value_type data_;
		// ReSharper restore CppRedundantInlineSpecifier
	};

	class memory_block;
	using memory_block_opt = std::optional<memory_block>;

	class memory_block final
	{
	public:
		using element_type = known_bytes_range::element_type;
		using value_type = known_bytes_range::value_type;
		using size_type = known_bytes_range::size_type;
		using difference_type = known_bytes_range::difference_type;
		using pointer = known_bytes_range::pointer;
		using const_pointer = known_bytes_range::const_pointer;
		using reference = known_bytes_range::reference;
		using const_reference = known_bytes_range::const_reference;
		using iterator = known_bytes_range::iterator;
		using reverse_iterator = known_bytes_range::reverse_iterator;

		memory_block( ) = default;

		memory_block(const address& begin, size_type mem_size);
		memory_block(const address& begin, const address& end);
		memory_block(const address& addr);

		explicit memory_block(const known_bytes_range& span);

	private:
		memory_block_opt find_block_impl(const known_bytes_range_const& rng) const;
		memory_block_opt find_block_impl(const unknown_bytes_range_const& rng) const;
		memory_block_opt find_block_impl(const any_bytes_range& rng) const;

	public:
		template <typename T>
		memory_block_opt find_block(const T& obj) const
		{
			if constexpr (std::same_as<T, any_bytes_range>)
			{
				return this->find_block_impl(obj);
			}
			else if constexpr (!std::ranges::range<T>)
			{
				static_assert(!std::is_pointer_v<T> && std::is_trivially_destructible_v<T>, __FUNCSIG__": Unsupported block data type!");
				const auto rng = known_bytes_range_const(reinterpret_cast<const known_byte*>(std::addressof(obj)), sizeof(T));
				return this->find_block_impl(rng);
			}
			else
			{
				static_assert(std::ranges::random_access_range<T>, __FUNCSIG__": Unsupported range type!");
				auto first = std::_Get_unwrapped(obj.begin( ));
				using raw_t = std::ranges::range_value_t<T>;
				if constexpr (is_std_optional_v<raw_t>)
				{
					static_assert(sizeof(typename raw_t::value_type) == sizeof(std::byte), __FUNCSIG__": Unsupported range element type!");
					const auto rng = unknown_bytes_range_const(reinterpret_cast<const unknown_byte*>(first), obj.size( ));
					return this->find_block_impl(rng);
				}
				else
				{
					static_assert(sizeof(raw_t) == sizeof(known_byte), __FUNCSIG__": Unsupported range element type!");
					const auto rng = known_bytes_range_const(reinterpret_cast<const known_byte*>(first), obj.size( ));
					return this->find_block_impl(rng);
				}
			}
		}

		template <typename T>
		std::vector<memory_block> find_all_blocks(const T& block) const
		{
			//todo: do it with std::ranges
			auto data = std::vector<memory_block>( );
			auto from = *this;
			for (;;)
			{
				memory_block_opt found_block = from.find_block(block);
				if (!found_block.has_value( ))
					break;
				from = from.shift_to_end(*found_block);
				data.push_back(std::move(*found_block));
			}
			return data;
		}

		///use find_all_blocks directly
		//std::vector<memory_block> find_xrefs(const address& addr) const;

		address addr( ) const;
		address last_addr( ) const;

		memory_block subblock(size_t offset) const;
		memory_block shift_to(pointer ptr) const;
		memory_block shift_to_start(const memory_block& block) const;
		memory_block shift_to_end(const memory_block& block) const;

		using flags_type = unsigned long;

		bool have_flags(flags_type flags) const;
		bool dont_have_flags(flags_type flags) const;

		bool readable( ) const;
		bool readable_ex( ) const;
		bool writable( ) const;
		bool executable( ) const;
		bool code_padding( ) const;

		const known_bytes_range& bytes_range( ) const;

	private:
		known_bytes_range bytes_;
	};
}
