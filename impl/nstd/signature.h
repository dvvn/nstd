#pragma once
#include "memory block.h"

namespace nstd
{
	enum class signature_parse_mode
	{
		/**
		 * \brief store string viewable data as TEXT all others as BYTES.
		 * while data is string viewable, even if all bytes are known, TEXT_AS_BYTES is never selected
		 */
		AUTO,
		/**
		 * \brief text converted to bytes and stored like raw memory block. all bytes must be known
		 */
		TEXT_AS_BYTES,
		/**
		 * \brief text converted to bytes and stored inside special container.
		 * unknown bytes allowed. when possible prefer to TEXT_AS_BYTES if all bytes known
		 */
		TEXT,
		/**
		 * \brief raw memory block. any type is stored like range of bytes
		 */
		BYTES,
	};

	namespace signature_detail
	{
		namespace rng=std::ranges;
		namespace rngv=rng::views;

		constexpr unknown_byte get_byte(known_byte chr)
		{
			struct char_and_number
			{
				known_byte number;
				known_byte character;
			};
			// ReSharper disable once CppInconsistentNaming
#define _C2N_(num) char_and_number(0x##num, #num[0])
			constexpr std::array data{
				_C2N_(0),_C2N_(1),_C2N_(2),_C2N_(3),_C2N_(4),_C2N_(5),_C2N_(6),_C2N_(7),_C2N_(8),_C2N_(9),
				_C2N_(a),_C2N_(b),_C2N_(c),_C2N_(d),_C2N_(e),_C2N_(f),
				_C2N_(A),_C2N_(B),_C2N_(C),_C2N_(D),_C2N_(E),_C2N_(F)
			};
#undef _C2N_
			for (const auto& [number, character]: data)
			{
				if (character == chr)
					return number;
			}
			return { };
			//magic numbers suck
			/*if (c >= '0' && c <= '9')
				return c - '0';
			if (c >= 'a' && c <= 'f')
				return c - 87;
			if (c >= 'A' && c <= 'F')
				return c - 55;

			return std::nullopt;*/
		}

		constexpr auto character_is_byte = []<typename Chr>(Chr c)
		{
			if constexpr (sizeof(Chr) == sizeof(known_byte))
				return true;
			else
				return (static_cast<size_t>(c) <= static_cast<size_t>(std::numeric_limits<known_byte>::max( )));
		};

		struct simulate_exceptrion final: std::exception
		{
			simulate_exceptrion(const char* const msg) : std::exception(msg)
			{
			}

			simulate_exceptrion( ) : std::exception( )
			{
			}
		};

		template <typename E, typename Tr>
		std::vector<E> get_beautiful_sig(const std::basic_string_view<E, Tr>& str)
		{
			auto sig = std::vector<E>( );
			sig.reserve(str.size( ));
			auto add_space = false;
			for (auto c: str)
			{
				if (c == static_cast<E>(' '))
				{
					if (add_space)
					{
						add_space = false;
						sig.push_back(c);
					}
				}
				else
				{
					add_space = true;
					sig.push_back(c);
				}
			}
			return sig;
		}

		template <typename E, typename Tr>
		bool is_sig_beautiful(const std::basic_string_view<E, Tr>& str)
		{
			constexpr auto space = static_cast<E>(' ');
			if (str.starts_with(space) || str.ends_with(space))
				return false;
			auto space_detected = false;
			for (auto chr: str)
			{
				if (chr != space)
					space_detected = false;
				else if (space_detected)
					return false;
				else
					space_detected = true;
			}
			return true;
		}

		template <typename In, typename Out>
		class sig_one_space
		{
		public:
			using in_t = std::vector<In>;
			using out_t = std::span<Out>;

			template <typename T>
			sig_one_space(T&& str)
			{
				if (is_sig_beautiful(str))
					data_.template emplace<out_t>(str);
				else
					data_.template emplace<in_t>(get_beautiful_sig(str));
			}

			out_t get( ) const
			{
				return std::visit(nstd::overload([](const in_t&  in) { return out_t(in); },
												 [](const out_t& out) { return out; }), data_);
			}

		private:
			std::variant<in_t, out_t> data_;
		};

		template <typename E, typename Tr>
		sig_one_space(const std::basic_string_view<E, Tr>&) -> sig_one_space<E, const E>;
		template <typename E, typename Tr>
		sig_one_space(std::basic_string_view<E, Tr>&) -> sig_one_space<E, E>;

#ifdef _DEBUG
#define NSTD_SIG_SIMULATE_MSG(msg) msg
#else
#define NSTD_SIG_SIMULATE_MSG(...) 
#endif

#define NSTD_SIG_SIMULATE(expr, msg)\
		if constexpr(Simulate)\
		{if(!!(expr) == false) throw simulate_exceptrion(NSTD_SIG_SIMULATE_MSG(msg));}\
		else\
		{runtime_assert(expr, msg);}

		//#define NSTD_SIG_SIMULATE_STATIC(expr, msg)\
		//		if constexpr(Simulate)\
		//		{if constexpr(!!(expr) == false) throw simulate_exceptrion(NSTD_SIG_SIMULATE_MSG(msg));}\
		//		else\
		//		{static_assert(expr, msg);}

		template <bool Simulate>
		struct helpers
		{
			template <typename E, typename Tr>
			static constexpr unknown_byte text_to_byte(const std::basic_string_view<E, Tr>& str)
			{
				switch (str.size( ))
				{
					case 1:
					{
						NSTD_SIG_SIMULATE(character_is_byte(str[0]), "Incorrect signature: unsupported unicode characted detected");
						const auto chr = static_cast<known_byte>(str[0]);
						if (chr == '?')
							return { };
						const auto ret = get_byte(chr);
						NSTD_SIG_SIMULATE(ret.has_value( ), "Incorrect signature: wrong byte");
						return ret;
					}
					case 2:
					{
						NSTD_SIG_SIMULATE(rng::all_of(str, character_is_byte), "Unable to convert unicode text to byte");
						const auto chr1 = static_cast<known_byte>(str[0]);
						const auto chr2 = static_cast<known_byte>(str[1]);
						if (chr1 == '?')
						{
							NSTD_SIG_SIMULATE(chr2 == '?', "Incorrect signature: second char in known whle first are unknown");
							return { };
						}
						NSTD_SIG_SIMULATE(chr2 != '?', "Incorrect signature: second char in unknown while first are known");
						const auto byte1 = get_byte(chr1);
						NSTD_SIG_SIMULATE(byte1.has_value( ), "Incorrect signature: wrong first byte")
						const auto byte2 = get_byte(chr2);
						NSTD_SIG_SIMULATE(byte2.has_value( ), "Incorrect signature: wrong second byte")
						return {static_cast<size_t>(*byte1) * static_cast<size_t>(16) + static_cast<size_t>(*byte2)};
					}
					default:
					{
						NSTD_SIG_SIMULATE(false, "Incorrect signature: whrong text size");
						return { };
					}
				}
			};

			template <typename E, typename Tr, typename Fn, typename Sv = std::basic_string_view<E, Tr>>
			static constexpr void parse_text_as_bytes(const Sv& str, Fn&& store_fn)
			{
#if 0
				const auto clamp_spaces = [&]
				{
					const auto spaces_end = str.find_first_not_of(' ');
					if(spaces_end != str.npos)
						str.remove_prefix(spaces_end);
				};

				clamp_spaces( );
				runtime_assert(!str.empty( ), "Signature range is empty!");

				while(!str.empty( ))
				{
					const auto end = str.find_first_of(' ');
					runtime_assert(end != str.npos || str.size( ) <= 2, "End of text byte not found");

					const auto part = str.substr(0, end);
					const auto byte = text_to_byte(part);

					std::invoke(store_fn, byte);
					if(end == str.npos)
						break;

					str.remove_prefix(end);
					clamp_spaces( );
				}

#endif
				NSTD_SIG_SIMULATE(!str.empty( ), "Incorrect signature: whrong text size");
				auto beautiful_sig      = sig_one_space(str);
				auto beautiful_sig_view = beautiful_sig.get( );
			
				for (auto part: beautiful_sig_view
								| rngv::split(static_cast<E>(' '))
								| rngv::transform([](auto&& rng) { return Sv(std::addressof(*rng.begin( )), rng::distance(rng)); }))
				{
					const auto byte = helpers<Simulate>::text_to_byte(part);
					std::invoke(store_fn, byte);
				}
			}
		};

		template <bool Simulate, signature_parse_mode M>
		struct maker;

		template <bool Simulate>
		struct maker<Simulate, signature_parse_mode::TEXT_AS_BYTES>
		{
			template <typename E, typename Tr>
			// ReSharper disable once CppRedundantInlineSpecifier
			_CONSTEXPR20_CONTAINER known_bytes_object operator()(const std::basic_string_view<E, Tr>& str) const
			{
				auto       vec      = known_bytes_object( );
				const auto store_fn = [&vec](const unknown_byte& b)
				{
					NSTD_SIG_SIMULATE(b.has_value( ), "Incorrect signature: unknown byte detected, while all bytes must be known in TEXT_AS_BYTES mode!");
					vec.push_back(*b);
				};
				helpers<Simulate>::template parse_text_as_bytes<E, Tr>(str, store_fn);
				return vec;
			}
		};

		template <bool Simulate>
		struct maker<Simulate, signature_parse_mode::TEXT>
		{
			template <typename E, typename Tr>
			// ReSharper disable once CppRedundantInlineSpecifier
			_CONSTEXPR20_CONTAINER unknown_bytes_object operator()(const std::basic_string_view<E, Tr>& str) const
			{
				auto       vec      = unknown_bytes_object( );
				const auto store_fn = [&](const unknown_byte& b)
				{
					vec.push_back(b);
				};
				helpers<Simulate>::template parse_text_as_bytes<E, Tr>(str, store_fn);
				return vec;
			}
		};

		template < >
		struct maker<false, signature_parse_mode::BYTES>
		{
			template <typename T>
			constexpr auto operator()(const T& val) const
			{
				
				if constexpr (!std::is_trivially_destructible_v<T>)
				{
					static_assert(std::_Always_false<T>, __FUNCSIG__": T must be trivially destructible");
#if 0
					NSTD_SIG_SIMULATE_STATIC(ranges::range<T>, "T must be a range");
					using rng_val = ranges::range_value_t<T>;
					NSTD_SIG_SIMULATE_STATIC(std::is_trivially_destructible_v<rng_val>, "T must be trivially destructible");

					auto vec = known_bytes_object( );

					for(auto& v : val)
					{
						for(auto b : std::invoke(*this, val)) //get span of 1..X elements
						{
							NSTD_SIG_SIMULATE_STATIC(sizeof(decltype(b)) == sizeof(known_byte), "Internal type must be a \"known_byte\"");
							vec.push_back(reinterpret_cast<known_byte>(b)); //reinterpret_cast because of std::byte
						}
					}

					return vec;
#endif
				}
				else
				{
					if constexpr (!rng::random_access_range<T>)
					{
						static_assert(!rng::range<T>, "T shouldn't be a range");
						return std::as_bytes(std::span(std::addressof(val), 1));
					}
					else
					{
						using rng_val = rng::range_value_t<T>;
						static_assert(!std::is_pointer_v<rng_val>, "T shouldn't be a pointer");
						if constexpr (sizeof(rng_val) != sizeof(known_byte))
						{
							return std::as_bytes(std::span(rng::data(val), rng::size(val)));
						}
						else
						{
							if constexpr (std::_Is_any_of_v<rng_val, char, char8_t>)
								return std::basic_string_view<rng_val>(val);
							else
								return std::span(val);
						}
					}
				}
			}
		};

		template <typename>
		_INLINE_VAR constexpr bool is_std_string_v = false;

		template <typename C, typename Tr>
		_INLINE_VAR constexpr bool is_std_string_v<std::basic_string_view<C, Tr>> = true;

		template <typename C, typename Tr>
		_INLINE_VAR constexpr bool is_std_string_v<std::basic_string<C, Tr>> = true;

		template < >
		struct maker<true, signature_parse_mode::AUTO>
		{
			template <typename T>
			constexpr auto operator()(const T& obj) const
			{
				if constexpr (!is_std_string_v<T>)
				{
					if constexpr (std::is_bounded_array_v<T> && std::_Is_any_of_v<rng::range_value_t<T>, char, char8_t, wchar_t, char16_t, char32_t>)
						return std::invoke(*this, std::basic_string_view(obj));
					else
						return std::invoke(maker<false, signature_parse_mode::BYTES>( ), obj);
				}
				else
				{
#if 0
					//memory block rewrap it to known_bytes_object if wanted, so no problems here
					const auto to_unknown = []<typename T1>(const T1 & bytes)
					{
						static_assert(sizeof(typename T1::value_type) == sizeof(known_byte));
						auto tmp = rngv::transform(bytes, [](auto b) { return unknown_byte(static_cast<known_byte>(b)); });
						return unknown_bytes_object(tmp.begin( ), tmp.end( ));
					};
#endif
					if (rng::all_of(obj, character_is_byte))
					{
						try
						{
							known_bytes_object bytes = std::invoke(maker<true, signature_parse_mode::TEXT_AS_BYTES>( ), obj);
							//return to_unknown(bytes);
							return any_bytes_range(std::move(bytes));
						}
						catch (const simulate_exceptrion& e)
						{
						}
						try
						{
							unknown_bytes_object bytes = std::invoke(maker<true, signature_parse_mode::TEXT>( ), obj);
							return any_bytes_range(std::move(bytes));
						}
						catch ([[maybe_unused]] const simulate_exceptrion& e)
						{
						}
					}
					auto bytes = std::invoke(maker<false, signature_parse_mode::BYTES>( ), obj);
					//return to_unknown(bytes);
					const auto start = reinterpret_cast<const known_byte*>(bytes._Unchecked_begin( ));
					const auto end   = reinterpret_cast<const known_byte*>(bytes._Unchecked_end( ));
					return any_bytes_range(known_bytes_range_const(start, end));
				}
			}
		};
	}

	template <signature_parse_mode M = signature_parse_mode::AUTO, typename T>
	/*_INLINE_VAR*/ constexpr auto make_signature /*= []<typename T>*/(const T& data)
	{
		return std::invoke(signature_detail::maker<M == signature_parse_mode::AUTO, M>( ), data);
	};
}
