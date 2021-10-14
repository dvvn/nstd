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

		template <typename Fn, typename Arg>
		auto ret_val(const Fn& fn, Arg& arg)
		{
			return fn(arg);
		}

		template <typename Q, typename Curr, typename ...A>
		auto do_invoke(Q&& obj, Curr&& fn, A&&...args)
		{
			//using fn_ret = std::invoke_result_t<Curr, Q>;//something wrong inside of this
			using fn_ret = decltype(ret_val(fn, obj));
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
				: addr(std::move(old.addr)), line(std::tuple_cat(std::move(old.line), std::make_tuple(std::forward<Tnew>(back))))

			{
			}

			template <not_address_pipe A, typename ...Args>
				requires((address_invocable<A> || std::convertible_to<A, address>) && address_invocable<Args...>)
			address_pipe_impl(A&& a, Args&&...args)
				: addr(std::forward<A>(a)), line(std::forward<Args>(args)...)
			{
			}

		private:
			T addr;
			std::tuple<Ts...> line;

#ifdef _DEBUG
			mutable bool done = false;
#endif
		public:
			auto get_addr() const
			{
				if constexpr (!std::invocable<T>)
					return addr;
				else
					return std::invoke(addr);
			}

			template <size_t ...I>
			auto unpack(std::index_sequence<I...>) const
			{
				return do_invoke(get_addr( ), std::get<I>(line)...);
			}

			auto operator()() const
			{
#ifdef _DEBUG
				runtime_assert(done == false);
				done = true;
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
		constexpr auto remove(ptrdiff_t offset)
		{
			return make_invoke_fn(&address::remove, offset);
		}
#else
		NSTD_MAKE_ADDRESS_PIPE(remove);
#endif
		NSTD_MAKE_ADDRESS_PIPE(multiply);
		NSTD_MAKE_ADDRESS_PIPE(divide);

		NSTD_MAKE_ADDRESS_PIPE(rel8);
		NSTD_MAKE_ADDRESS_PIPE(rel32);

		NSTD_MAKE_ADDRESS_PIPE(jmp);

#undef NSTD_MAKE_ADDRESS_PIPE

#if 0
		template <typename T>
		auto add(const T& value)
		{
			return [=](address& addr)
			{
				addr += value;
			};
		}

		template <typename T>
		auto remove(const T& value)
		{
			return [=](address& addr)
			{
				addr -= value;
			};
		}

		template <typename T>
		auto multiply(const T& value)
		{
			return [=](address& addr)
			{
				addr *= value;
			};
		}

		template <typename T>
		auto divide(const T& value)
		{
			return [=](address& addr)
			{
				addr /= value;
			};
		}

		inline auto deref(size_t count)
		{
			return [=](const address& addr)
			{
				return addr.deref(count);
			};
		}

		_INLINE_VAR constexpr auto value = [](const address& addr)
		{
			return addr.value( );
		};

		template <typename T>
		_INLINE_VAR constexpr auto cast = [](const address& addr) -> T
		{
			return addr.cast<T>( );
		};

		template <typename T>
		_INLINE_VAR constexpr auto ref = nstd::overload([](const address& addr) -> const T&
		{
			return addr.ref<T>( );
		},
			[](address& addr) -> T&
		{
			return addr.ref<T>( );
		});

		template <typename T>
		_INLINE_VAR constexpr auto ptr = [](const address& addr) -> T*
		{
			return addr.ptr<T>( );
		};

		inline auto jmp(ptrdiff_t offset = 0x1)
		{
			return [=](const address& addr)
			{
				return addr.jmp(offset);
			};
		}

#endif

		template <typename T, typename ...Ts>
		address_pipe_impl(address_pipe_impl<Ts...>&& old, T&& val) -> address_pipe_impl<Ts..., std::remove_cvref_t<T>>;

		template <not_address_pipe A, typename ...B>
		address_pipe_impl(A&& a, B&& ...b) -> address_pipe_impl<std::remove_cvref_t<A>, std::remove_cvref_t<B>...>;

		template <typename A, typename B>
			requires(std::constructible_from<address_pipe_impl<std::remove_cvref_t<A>, std::remove_cvref_t<B>>, A, B>)
		auto operator|(A&& a, B&& b)
		{
			return address_pipe_impl(std::forward<A>(a), std::forward<B>(b));
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
			auto operator|(Fn&& addr, T&& obj)
		{
			return address_pipe_impl<std::remove_cvref_t<Fn>, T>
				(
					std::forward<Fn>(addr), std::make_tuple(std::forward<T>(obj))
					);
		}
#endif
	}

	template <typename ...Ts>
	auto apply_address_pipe(nstd::address addr, Ts&& ...args)
	{
		if constexpr (sizeof...(Ts) == 0)
			return addr;
		else
		{
			auto pipe = address_pipe::address_pipe_impl<address, std::remove_cvref_t<Ts>...>((addr), std::forward<Ts>(args)...);
			return std::invoke(pipe);
		}
	}
}
