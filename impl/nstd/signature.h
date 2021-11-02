#pragma once
#include "runtime_assert_fwd.h"

#include <span>
#include <vector>

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
	class signature_known_bytes : protected StorageType
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

	class signature_unknown_bytes : signature_known_bytes<std::vector<uint8_t>>
	{
	public:
		using signature_known_bytes::begin;
		using signature_known_bytes::end;
		using signature_known_bytes::storage;

		signature_unknown_bytes( ) = default;

		class writer
		{
			signature_unknown_bytes* storage_;

		public:
			writer(signature_unknown_bytes* const storage)
				: storage_(storage)
			{
			}

			void operator()( ) const
			{
				++storage_->skip;
			}

			void operator()(uint8_t byte)
			{
				if (storage_->skip > 0)
				{
					storage_->next = std::make_unique<signature_unknown_bytes>( );
					storage_       = storage_->next.get( );
				}
				storage_->push_back(byte);
			}
		};

		writer get_writer( )
		{
			return writer{this};
		}

		size_t skip = 0;
		std::unique_ptr<signature_unknown_bytes> next;
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
		constexpr void text_to_bytes(Itr begin, Itr end, WriterKnown wknown, WriterUnknown wunk)
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
#define NSTD_SIG_SET_BYTE(_BYTE_) \
	case #_BYTE_[0] :\
		set_byte(0x##_BYTE_);\
		break
				switch (*itr)
				{
					case ' ':
					{
						write_byte( );
						bytes_unknown = 0;
						break;
					}
					case '?':
					{
						write_byte( );
						write_unknown( );
						break;
					}
					NSTD_SIG_SET_BYTE(0);
					NSTD_SIG_SET_BYTE(1);
					NSTD_SIG_SET_BYTE(2);
					NSTD_SIG_SET_BYTE(3);
					NSTD_SIG_SET_BYTE(4);
					NSTD_SIG_SET_BYTE(5);
					NSTD_SIG_SET_BYTE(6);
					NSTD_SIG_SET_BYTE(7);
					NSTD_SIG_SET_BYTE(8);
					NSTD_SIG_SET_BYTE(9);
						//-
					NSTD_SIG_SET_BYTE(a);
					NSTD_SIG_SET_BYTE(b);
					NSTD_SIG_SET_BYTE(c);
					NSTD_SIG_SET_BYTE(d);
					NSTD_SIG_SET_BYTE(e);
					NSTD_SIG_SET_BYTE(f);
						//-
					NSTD_SIG_SET_BYTE(A);
					NSTD_SIG_SET_BYTE(B);
					NSTD_SIG_SET_BYTE(C);
					NSTD_SIG_SET_BYTE(D);
					NSTD_SIG_SET_BYTE(E);
					NSTD_SIG_SET_BYTE(F);
					default:
						runtime_assert("Unsupported character");
				}

#undef NSTD_SIG_SET_BYTE
			}

			write_byte( );
		}
	}

	//todo: char[X] version
	//todo chars_cache version
	template <std::input_iterator Itr, auto Tag = detail::make_signature_tag_selector<Itr>( )>
	constexpr auto make_signature(Itr begin, Itr end)
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
					return make_storage(signature_known_bytes<std::vector<uint8_t>>( ));
			}
		}
	}

	template <class T>
		requires(!std::ranges::range<T> && std::is_trivially_destructible_v<T>)
	constexpr auto make_signature(T&& val)
		requires(!std::is_rvalue_reference_v<decltype(val)>)
	{
		auto rng = std::span(reinterpret_cast<const uint8_t*>(std::addressof(val)), sizeof(T));
		return make_signature(rng.begin( ), rng.end( ));
	}

	template <auto Tag = std::false_type( ), std::ranges::range T>
	constexpr auto make_signature(const T& val)
	{
		using itr = std::ranges::iterator_t<T>;
		static_assert(sizeof(std::iter_value_t<T>) == sizeof(uint8_t));

		constexpr auto tag = []
		{
			if constexpr (std::same_as<std::remove_const_t<decltype(Tag)>, std::false_type>)
				return detail::make_signature_tag_selector<itr>( );
			else
				return Tag;
		}( );

		using tag_type = std::remove_const_t<decltype(tag)>;

		if constexpr (std::same_as<tag_type, make_signature_tag_direct>)
		{
			static_assert(std::ranges::random_access_range<T>,"Unsupported range type");
			auto begin = reinterpret_cast<const uint8_t*>(std::ranges::_Ubegin(val));
			auto end   = reinterpret_cast<const uint8_t*>(std::ranges::_Uend(val));
			return make_signature<const uint8_t*, tag>(std::move(begin), std::move(end));
		}
		else if constexpr (std::same_as<tag_type, make_signature_tag_convert>)
		{
			auto begin = std::ranges::begin(val);
			auto end   = std::ranges::end(val);
			return make_signature<decltype(begin), tag>(std::move(begin), std::move(end));
		}
	}
}
