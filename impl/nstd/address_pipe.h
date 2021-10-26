#pragma once
#include "address.h"
#include "runtime_assert_fwd.h"

#include <concepts>
#include <tuple>

namespace nstd
{
	namespace address_pipe
	{
		struct address_pipe_tag
		{
		};

		template <typename Q, typename Curr, typename ...A>
		auto do_invoke(Q&& obj, Curr&& fn, A&&...args)
		{
			//using fn_ret = std::invoke_result_t<Curr, Q>;//something wrong inside of this
			using fn_ret = decltype(fn(std::forward<Q>(obj)));
			constexpr auto args_count = sizeof...(A);
			if constexpr (std::is_void_v<fn_ret>)
			{
				std::invoke(fn, obj);
				if constexpr (args_count > 0)
					return do_invoke(std::forward<Q>(obj), std::forward<A>(args)...);
				else
					return obj;
			}
			else
			{
				auto new_obj = std::invoke(fn, obj);
				if constexpr (args_count > 0)
					return do_invoke(std::move(new_obj), std::forward<A>(args)...);
				else
					return new_obj;
			}
		}

		template <typename ...Ts>
		concept address_invocable = (std::invocable<Ts, address> && ...);

		template <typename T>
		concept not_address_pipe = !std::derived_from<std::remove_cvref_t<T>, address_pipe_tag>;

		template <typename T, typename ...Ts>
		struct address_pipe_impl : address_pipe_tag
		{
			template <address_invocable Tnew, typename ...Args>
			address_pipe_impl(address_pipe_impl<T, Args...>&& old, Tnew&& back)
				: addr_(std::move(old.addr_)), pipe_(std::tuple_cat(std::move(old.pipe_), std::make_tuple(std::forward<Tnew>(back))))

			{
			}

			template <not_address_pipe A, typename ...Args>
				requires((address_invocable<A> || std::convertible_to<A, address>) && address_invocable<Args...>)
			address_pipe_impl(A&& a, Args&&...args)
				: addr_(std::forward<A>(a)), pipe_(std::forward<Args>(args)...)
			{
			}

		private:
			T addr_;
			std::tuple<Ts...> pipe_;

#ifdef _DEBUG
			mutable bool done_ = false;
#endif
		public:
			auto get_addr( ) const
			{
				if constexpr (!std::invocable<T>)
					return addr_;
				else
					return std::invoke(addr_);
			}

			template <size_t ...I>
			auto unpack(std::index_sequence<I...>) const
			{
				return do_invoke(get_addr( ), std::get<I>(pipe_)...);
			}

			auto operator()( ) const
			{
#ifdef _DEBUG
				runtime_assert(done_ == false);
				done_ = true;
#endif
				return unpack(std::index_sequence_for<Ts...>( ));
			}
		};

		//----

		template <typename Addr, typename Fn, typename ...Args>
		concept address_invocable_bind = std::invocable<Fn, Addr, Args...>;

		template <typename Fn, typename ...Ts>
		constexpr auto make_invoke_fn(Fn&& fn, Ts&&...args)
		{
			return [=]<address_invocable_bind<Fn, Ts...> Addr>(Addr&& addr)-> decltype(auto)
			{
				return std::invoke(fn, addr, args...);
			};
		};

		template <typename Ret, typename ...Args>
		constexpr auto bind_to_address_front(Ret (address::* fn)(Args ...) const)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				return make_invoke_fn(fn);
			}
			else
			{
				return [=](Args ...args)
				{
					return make_invoke_fn(fn, args...);
				};
			}
		}

		template <typename Ret, typename ...Args>
		constexpr auto bind_to_address_front(Ret (address::* fn)(Args ...))
		{
			if constexpr (sizeof...(Args) == 0)
			{
				return make_invoke_fn(fn);
			}
			else
			{
				return [=](Args ...args)
				{
					return make_invoke_fn(fn, args...);
				};
			}
		}

#define NSTD_MAKE_ADDRESS_PIPE(_FN_,...)\
	_INLINE_VAR constexpr auto _FN_ = bind_to_address_front(&address::_FN_##__VA_ARGS__)

		NSTD_MAKE_ADDRESS_PIPE(value);

		template <typename T>
		NSTD_MAKE_ADDRESS_PIPE(cast, <T>);
		template <typename T>
		NSTD_MAKE_ADDRESS_PIPE(ref, <T>);

		NSTD_MAKE_ADDRESS_PIPE(deref);
		NSTD_MAKE_ADDRESS_PIPE(deref_safe);

		NSTD_MAKE_ADDRESS_PIPE(add);
#ifdef _INC_STDIO
		constexpr auto remove(ptrdiff_t offset) { return make_invoke_fn(&address::remove, offset); }
#else
		NSTD_MAKE_ADDRESS_PIPE(remove);
#endif
		NSTD_MAKE_ADDRESS_PIPE(multiply);
		NSTD_MAKE_ADDRESS_PIPE(divide);

		NSTD_MAKE_ADDRESS_PIPE(rel8);
		NSTD_MAKE_ADDRESS_PIPE(rel32);

		NSTD_MAKE_ADDRESS_PIPE(jmp);

#undef NSTD_MAKE_ADDRESS_PIPE

		struct invoke_tag
		{
		};

		_INLINE_VAR constexpr auto invoke = invoke_tag( );

		//-----------------

		template <typename T, typename ...Ts>
		address_pipe_impl(address_pipe_impl<Ts...>&& old, T&& val) -> address_pipe_impl<Ts..., std::remove_cvref_t<T>>;

		template <not_address_pipe A, typename ...B>
		address_pipe_impl(A&& a, B&& ...b) -> address_pipe_impl<std::remove_cvref_t<A>, std::remove_cvref_t<B>...>;

		template <std::derived_from<address_pipe_tag> A, typename B>
		auto operator|(A&& a, B&& b)
		{
			return address_pipe_impl(std::forward<A>(a), std::forward<B>(b));
		}

		template <std::derived_from<address> A, typename B>
		auto operator|(A&& a, B&& b)
		{
			return address_pipe_impl(std::forward<A>(a), std::forward<B>(b));
		}

		//----

		template <std::invocable T>
		auto operator |(T&& obj, invoke_tag)
		{
			return std::invoke(std::forward<T>(obj));
		}

		template <std::derived_from<address> T>
		auto operator |(T&& addr, invoke_tag)
		{
			return addr;
		}

#if 0
		template <typename Tpl, typename T, size_t ...Idx>
		auto tuple_cat_custom(Tpl&& tpl, T&& val, std::index_sequence<Idx...>)
		{
			return std::make_tuple(std::move(std::get<Idx>(tpl))..., std::forward<T>(val));
		}

		template <typename T, typename ...Ts>
		auto operator|(address_pipe_impl<Ts...>&& pipe, T&& obj)
		{
			return address_pipe_impl(std::move(pipe), std::forward<T>(obj));
		}

		template <std::invocable Fn, typename T>
		requires(!std::derived_from<Fn, address_pipe_tag>)
			auto operator|(Fn&& addr_, T&& obj)
		{
			return address_pipe_impl<std::remove_cvref_t<Fn>, T>
				(
					std::forward<Fn>(addr_), std::make_tuple(std::forward<T>(obj))
					);
		}
#endif
	}
}
