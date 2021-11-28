#pragma once
#include "runtime_assert_fwd.h"

#include <span>
#include <vector>
#include <string>

namespace nstd
{
	namespace detail
	{
		template <typename T>
		concept have_push_back = requires(T obj)
		{
			obj.push_back(std::declval<typename T::value_type>( ));
		};

		template <typename T>
		concept have_insert = requires(T obj)
		{
			obj.insert(obj.begin( ), std::declval<typename T::value_type>( ));
		};
	}

	template <class StorageType>
	class signature_known_bytes final : protected StorageType
	{
	public:
		using StorageType::begin;
		using StorageType::end;
		using StorageType::size;

		template <typename ...Args>
		signature_known_bytes(Args&&...args)
			: StorageType(std::forward<Args>(args)...)
		{
		}

		signature_known_bytes( ) = default;

		class writer
		{
			signature_known_bytes* storage_;
			typename StorageType::size_type offset_ = 0;

		public:
			writer(signature_known_bytes* const storage)
				: storage_(storage)
			{
			}

			void operator()( ) const
			{
				runtime_assert("Unable to store unknown byte");
			}

			void operator()(uint8_t byte)
			{
				if constexpr (detail::have_push_back<StorageType>)
					storage_->push_back(byte);
				else
				{
					auto itr = std::next(storage_->begin( ), ++offset_);
					if constexpr (detail::have_insert<StorageType>)
						storage_->insert(itr, byte);
					else
						*itr = byte;
				}
			}
		};

		writer get_writer( )
		{
			return writer{this};
		}

		const StorageType& storage( ) const { return *static_cast<const StorageType*>(this); }
	};

	template <class Storage>
	class signature_unknown_bytes_impl : Storage
	{
		//first : known bytes rng
		//second: bytes to skip count

	public:
		using Storage::begin;
		using Storage::end;
		using Storage::_Unchecked_begin;
		using Storage::_Unchecked_end;
		using Storage::empty;
		using Storage::size;
		using Storage::operator[];

		signature_unknown_bytes_impl( ) = default;

		class writer
		{
			signature_unknown_bytes_impl* storage_;
			typename Storage::size_type index_;

			auto& data( ) const
			{
				return (*storage_)[index_];
			}

			auto& _Known( )
			{
				return data( ).first;
			}

			auto& _Skip( )
			{
				return data( ).second;
			}

			void _Add_part(bool force)
			{
				if (force || _Skip( ) > 0)
				{
					storage_->emplace_back( );
					++index_;
				}
			}

		public:
			writer(signature_unknown_bytes_impl* const storage)
				: storage_(storage), index_(static_cast<size_t>(-1))
			{
				runtime_assert(storage_->empty());
				_Add_part(true);
			}

			void operator()( )
			{
				++_Skip( );
			}

			void operator()(uint8_t byte)
			{
				_Add_part(false);
				_Known( ).push_back(byte);
			}
		};

		writer get_writer( )
		{
			return this;
		}
	};

	using storage_for_bytes = std::basic_string<uint8_t, std::_Narrow_char_traits<uint8_t, uint32_t>>;

	class signature_unknown_bytes : public signature_unknown_bytes_impl<std::vector<std::pair<storage_for_bytes, size_t>>>
	{
	};

	//from bytes
	struct make_signature_tag_direct
	{
	};

	//from string like "01 02 A1 9 ?? ?"
	struct make_signature_tag_convert
	{
		constexpr make_signature_tag_convert(bool unknown_bytes = true, size_t reserved_size = 0)
			: unknown_bytes(unknown_bytes), reserved_size(reserved_size)
		{
		}

		bool unknown_bytes; // "01 ?? A1"
		size_t reserved_size;
	};

	namespace detail
	{
		template <typename T, bool Dereferenced = false>
		constexpr auto make_signature_tag_selector( )
		{
			if constexpr (!Dereferenced)
			{
				if constexpr (std::_Dereferenceable<T>)
					return make_signature_tag_selector<std::remove_cvref_t<decltype(*std::declval<T>( ))>, true>( );
			}
			else
			{
				if constexpr (std::same_as<uint8_t, T>)
					return make_signature_tag_direct( );
				else if constexpr (std::same_as<char, T>)
					return make_signature_tag_convert( );
			}
		}

		template <typename Itr, typename WriterKnown, typename WriterUnknown>
		constexpr void text_to_bytes(Itr begin, Itr end, WriterKnown&& wknown, WriterUnknown&& wunk)
		{
			uint8_t bytes_unknown = 0;
			uint8_t bytes_added   = 0;
			uint8_t last_byte;

			const auto set_byte = [&](uint8_t byte)
			{
				runtime_assert(bytes_unknown == 0, "Prev byte part are unknown");
				switch (bytes_added)
				{
					case 0:
						last_byte = byte;
						++bytes_added;
						break;
					case 1:
						last_byte *= 16;
						last_byte += byte;
						++bytes_added;
						break;
					default:
						runtime_assert("Too much bytes to add");
						break;
				}
			};
			const auto write_byte = [&]
			{
				switch (bytes_added)
				{
					case 0:
						break;
					case 1:
					case 2:
						runtime_assert(bytes_unknown == 0, "Prev byte part are unknown");
						std::invoke(wknown, last_byte);
						bytes_added = 0;
						break;
					default:
						runtime_assert("Too much bytes to add");
						break;
				}
			};
			const auto write_unknown = [&]
			{
				runtime_assert(bytes_unknown <= 2, "Too much unknown bytes!");
				std::invoke(wunk);
				++bytes_unknown;
			};

			for (auto itr = begin; itr != end; ++itr)
			{
				switch (*itr)
				{
					case ' ':
						write_byte( );
						bytes_unknown = 0;
						break;
					case '?':
						write_byte( );
						write_unknown( );
						break;
					case '0':
						set_byte(0x0);
						break;
					case '1':
						set_byte(0x1);
						break;
					case '2':
						set_byte(0x2);
						break;
					case '3':
						set_byte(0x3);
						break;
					case '4':
						set_byte(0x4);
						break;
					case '5':
						set_byte(0x5);
						break;
					case '6':
						set_byte(0x6);
						break;
					case '7':
						set_byte(0x7);
						break;
					case '8':
						set_byte(0x8);
						break;
					case '9':
						set_byte(0x9);
						break;
					case 'a':
					case 'A':
						set_byte(0xA);
						break;
					case 'b':
					case 'B':
						set_byte(0xB);
						break;
					case 'c':
					case 'C':
						set_byte(0xC);
						break;
					case 'd':
					case 'D':
						set_byte(0xD);
						break;
					case 'e':
					case 'E':
						set_byte(0xE);
						break;
					case 'f':
					case 'F':
						set_byte(0xF);
						break;
					default:
						runtime_assert("Unsupported character");
				}
			}

			write_byte( );
		}
	}

	//todo: char[X] version
	//todo chars_cache version
	template <std::input_iterator Itr, auto Tag = detail::make_signature_tag_selector<Itr>( )>
	constexpr auto make_signature_impl(Itr begin, Itr end)
	{
		using tag_type = std::remove_const_t<decltype(Tag)>;

		if constexpr (std::same_as<tag_type, make_signature_tag_direct>)
		{
			using storage = signature_known_bytes<std::span<const uint8_t>>;
			return storage(begin, end);
		}
		else if constexpr (std::same_as<tag_type, make_signature_tag_convert>)
		{
			const auto make_storage = [&]<class Storage>(Storage&& storage)
			{
				auto writer = storage.get_writer( );
				detail::text_to_bytes(begin, end, [&](uint8_t byte) { writer(byte); }, [&] { writer( ); });
				return storage;
			};

			if constexpr (Tag.unknown_bytes)
			{
				return make_storage(signature_unknown_bytes( ));
			}
			else
			{
				if constexpr (Tag.reserved_size > 0)
					return make_storage(signature_known_bytes<std::array<uint8_t, Tag.reserved_size>>( ));
				else
					return make_storage(signature_known_bytes<storage_for_bytes>( ));
			}
		}
	}

	template <class T>
	class bytes_view_impl
	{
	public:
		template <typename Q>
		bytes_view_impl(Q&& object)
			: object_(std::forward<Q>(object))
		{
		}

		const uint8_t* begin( ) const
		{
			return reinterpret_cast<const uint8_t*>(std::addressof(object_));
		}

		// ReSharper disable once CppMemberFunctionMayBeStatic
		constexpr size_t size( ) const
		{
			return sizeof(std::remove_cvref_t<T>);
		}

		const uint8_t* end( ) const
		{
			return begin( ) + size( );
		}

	private:
		T object_;
	};

	template <class T>
	struct bytes_view;

	template <class T>
	struct bytes_view<T&&> : bytes_view_impl<T>
	{
		bytes_view(T&& object)
			: bytes_view_impl<T>(std::move(object))
		{
			static_assert(std::is_trivially_move_constructible_v<T>,"Unable to construct bytes_view from rvalue");
		}
	};

	template <class T>
	struct bytes_view<const T&> : bytes_view_impl<const T&>
	{
		bytes_view(const T& object)
			: bytes_view_impl<const T&>(/*std::addressof*/object)
		{
		}
	};

	template <class T>
	struct bytes_view<T&> : bytes_view<const T&>
	{
		bytes_view(T& object)
			: bytes_view<const T&>(object)
		{
		}
	};

	template <typename T>
	bytes_view(T&&) -> bytes_view<T&&>
	{
	}

	template <class T>
		requires(std::is_trivially_destructible_v<T> && (std::is_bounded_array_v<std::remove_cvref_t<T>> || !std::ranges::range<T>))
	constexpr auto make_signature(T&& val)
	{
		//auto rng = std::span(reinterpret_cast<const uint8_t*>(std::addressof(val)), sizeof(T));
		//return make_signature(rng.begin( ), rng.end( ));
		return bytes_view(std::forward<T>(val));
	}

	namespace detail
	{
		template <class Itr>
		auto unwrap_and_cast(Itr b, Itr e)
		{
			static_assert(std::random_access_iterator<Itr>, "Iterator must have random access array");
			static_assert(std::_Unwrappable_v<Itr>,"Unable to unwrap the iterator");

			//.[u]nwrapped
			//.[b]yte

			const auto bu = std::_Get_unwrapped(b);
			const auto eu = std::_Get_unwrapped(e);
			const auto bb = reinterpret_cast<const uint8_t*>(bu);
			const auto eb = reinterpret_cast<const uint8_t*>(eu);

			return std::make_pair(bb, eb);
		}
	}

	template <auto Tag = std::false_type( ), class Itr>
	constexpr auto make_signature(Itr begin, Itr end)
	{
		constexpr auto tag = []
		{
			if constexpr (std::same_as<std::remove_const_t<decltype(Tag)>, std::false_type>)
				return detail::make_signature_tag_selector<Itr>( );
			else
				return Tag;
		}( );

		using tag_type = std::remove_const_t<decltype(tag)>;

		if constexpr (std::same_as<tag_type, make_signature_tag_direct>)
		{
			static_assert(sizeof(std::iter_value_t<Itr>) == sizeof(uint8_t));
			auto [begin1,end1] = detail::unwrap_and_cast(begin, end);
			return make_signature_impl<const uint8_t*, tag>(begin1, end1);
		}
		else if constexpr (std::same_as<tag_type, make_signature_tag_convert>)
		{
			return make_signature_impl<decltype(begin), tag>(std::move(begin), std::move(end));
		}
		else
		{
			//try convert it to bytes array
			auto [begin1,end1] = detail::unwrap_and_cast(begin, end);
			return make_signature_impl(begin1, end1);
		}
	}

	template <class Itr>
	constexpr auto make_signature(Itr begin, Itr end, make_signature_tag_direct)
	{
		constexpr make_signature_tag_direct gap;
		return make_signature<gap>(begin, end);
	}
}
