module;

#include "address_includes.h"

export module nstd.mem.address;

template <typename From, typename To>
concept reinterpret_convertible_to = requires(From val)
{
	reinterpret_cast<To>(val);
};

template <typename From, typename To>
concept static_convertible_to = requires(From val)
{
	static_cast<To>(val);
};

template<typename Out, typename In>
Out force_cast(In in)
{
	if constexpr (reinterpret_convertible_to<In, Out>)
	{
		return reinterpret_cast<Out>(in);
	}
	else if constexpr (reinterpret_convertible_to<In, uintptr_t>)
	{
		auto tmp = reinterpret_cast<uintptr_t>(in);
		return reinterpret_cast<Out>(tmp);
	}
	else
	{
		return reinterpret_cast<Out>(reinterpret_cast<uintptr_t&>(in));
	}
}

export namespace nstd::inline mem
{
#define NSTD_ADDRESS_OPERATOR_HEAD\
	template<typename L, typename R>\
	requires(have_address_tag<L> && address_constructible<R, L> || have_address_tag<R> && address_constructible<L, R>)

#define NSTD_ADDRESS_OPERATOR_MATH(_OP_)\
	NSTD_ADDRESS_OPERATOR_HEAD\
	L& operator##_OP_##=(L& left, R right) noexcept\
	{\
		_Unwrap_address_value(left) _OP_##= _Unwrap_address_value(right);\
		return left;\
	}\
	NSTD_ADDRESS_OPERATOR_HEAD\
	L operator##_OP_##(L left, R right) noexcept\
	{\
		_Unwrap_address_value(left) _OP_##= _Unwrap_address_value(right);\
		return left;\
	}

#define NSTD_ADDRESS_OPERATOR_EQUALITY(_OP_)\
	NSTD_ADDRESS_OPERATOR_HEAD\
	bool operator##_OP_##(L left, R right) noexcept\
	{\
		return _Unwrap_address_value(left) _OP_ _Unwrap_address_value(right);\
	}

	struct address_tag { };

	template<class T>
	concept have_address_tag = std::derived_from<T, address_tag>;
	template<typename T, typename Addr>
	concept address_constructible = std::constructible_from<Addr, T>;

	template<typename T>
	struct basic_address : address_tag
	{
		// class size is only 4 bytes on x86-32 and 8 bytes on x86-64.

		using size_type = uintptr_t;
		using difference_type = ptrdiff_t;
		using value_type = /*std::remove_const_t<T>*/T;
		using pointer_type = std::add_pointer_t<T>;

		union
		{
			size_type value;
			pointer_type pointer;
		};

		basic_address( ) :value(0) { }
		basic_address(size_type val) : value(val) { }
		basic_address(std::nullptr_t) : pointer(nullptr) { }
		basic_address(pointer_type ptr) : pointer(ptr) { }

		template<have_address_tag Q>
		basic_address(Q&& other) noexcept
			: value(other.value)
		{
		}

		template<have_address_tag Q>
		basic_address& operator=(Q&& other) noexcept
		{
			value = other.value;
			return *this;
		}

		template<size_type Count>
		basic_address deref( )const
		{
			const basic_address addr = *force_cast<pointer_type*>(pointer);
			if constexpr (Count == 1)
				return addr;
			else
				return addr.deref<Count - 1>( );
		}

		basic_address deref(difference_type count)const
		{
			runtime_assert(count > 0, "Count must be larger than zero!");

			auto ret = *this;
			while (count-- > 0)
			{
				ret = ret.deref<1>( );
			}
			return ret;
		}

		pointer_type operator->( )const
		{
			return pointer;
		}

		template <typename Q>
		operator Q* () const
		{
			return force_cast<Q*>(value);
		}

		template <typename Q>
		operator Q& () const
		{
			return *force_cast<Q*>(value);
		}

		basic_address operator*( )const
		{
			return deref<1>( );
		}

#if 0
		template <typename Q>
		auto cast( ) const
		{
			if constexpr (reinterpret_convertible_to<uintptr_t, Q>)
			{
				return reinterpret_cast<Q>(value);
			}
			else if constexpr (std::is_member_pointer_v<Q>)
			{
				Q ret;
				reinterpret_cast<pointer&>(ret) = pointer;
				return ret;
			}
	}
#endif

#if 0
		template <typename Q>
		auto ptr( ) const
		{
			//return cast<Q*>( );
			//basic_address<Q> tmp = value;
			//return tmp.pointer;
			return pointer.get<Q>( );
		}

		/*ptr_auto_cast<size_type> ptr( ) const
		{
			return value;
		}*/

		template <typename Q>
		auto& ref( ) const
		{
			return *ptr<Q>( );
		}

		ref_auto_cast<size_type> ref( ) const
		{
			return value;
}
#endif

		//-----

		template<typename Q>
		basic_address<Q> as( )const
		{
			return value;
		}

		template<typename Q>
		Q get( )const
		{
			return force_cast<Q>(value);
		}

		basic_address add(difference_type offset)const
		{
			return value + static_cast<size_type>(offset);
		}
		basic_address& add_self(difference_type offset)
		{
			value += static_cast<size_type>(offset);
			return *this;
		}

		basic_address remove(difference_type offset)const
		{
			return value - static_cast<size_type>(offset);
		}

		basic_address<void> jmp(difference_type offset = 0x1) const
		{
			//same as rel 32

			// Example:
			// E9 ? ? ? ?
			// The offset has to skip the E9 (JMP) instruction
			// Then deref the address coming after that to get to the function
			// Since the relative JMP is based on the next instruction after the basic_address it has to be skipped

			// Base address is the address that follows JMP ( 0xE9 ) instruction
			basic_address<void> base = add(offset);

			// Store the displacement
			// Note: Displacement address can be signed
			int32_t displacement = *base;

			// The JMP is based on the instruction after the basic_address
			// so the basic_address size has to be added
			// Note: This is always 4 bytes, regardless of architecture
			base.add_self(sizeof(uint32_t));

			// Now finally do the JMP by adding the function basic_address
			base.add_self(displacement);

			return base;
		}
	};

	template<typename T>
	basic_address(T*)->basic_address</*std::remove_cv_t<T>*/T>;

	template<typename T>
	auto& _Unwrap_address_value(T& obj)
	{
		if constexpr (have_address_tag<std::remove_cv_t<T>>)
			return obj.value;
		else
			return reinterpret_cast<basic_address<void>&>(obj).value;
	}

	template<typename L, typename R>
	using select_address_t = std::conditional_t<have_address_tag<L>, L, R>;

	NSTD_ADDRESS_OPERATOR_MATH(+);
	NSTD_ADDRESS_OPERATOR_MATH(-);
	NSTD_ADDRESS_OPERATOR_MATH(*);
	NSTD_ADDRESS_OPERATOR_MATH(/ );
	NSTD_ADDRESS_OPERATOR_EQUALITY(< );
	NSTD_ADDRESS_OPERATOR_EQUALITY(<= );
	NSTD_ADDRESS_OPERATOR_EQUALITY(> );
	NSTD_ADDRESS_OPERATOR_EQUALITY(>= );
	NSTD_ADDRESS_OPERATOR_EQUALITY(== );

	using address = basic_address<void>;

	/*template<typename Cast, typename Target>
	struct address_auto_cast :Target
	{
		template<typename T>
		address_auto_cast(basic_address<T> addr) :Target(addr.get<Cast>( ))
		{
		}
	};*/

#if 0

	template<typename T>
	concept address_value = std::constructible_from<address, T> && !std::derived_from<T, address>;

#define NSTD_ADDRESS_OPERATOR_OUT(_OP_) \
template<address_value T>\
constexpr T operator##_OP_##(T value, address addr)\
{\
	return (address(value) _OP_ addr)._Unwrap<T>( );\
}\

	NSTD_ADDRESS_OPERATOR_OUT(+);
	NSTD_ADDRESS_OPERATOR_OUT(-);
	NSTD_ADDRESS_OPERATOR_OUT(*);
	NSTD_ADDRESS_OPERATOR_OUT(/ );

	template<address_value T>
	constexpr auto operator<=>(T value, address addr)
	{
		return address(value) <=> addr;
	}

#endif

	//------------

#if 0

	//must be rewritten

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
			return[=]<address_invocable_bind<Fn, Ts...> Addr>(Addr && addr)-> decltype(auto)
			{
				return std::invoke(fn, addr, args...);
			};
		};

		template <typename Ret, typename ...Args>
		constexpr auto bind_to_address_front(Ret(address::* fn)(Args ...) const)
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
		constexpr auto bind_to_address_front(Ret(address::* fn)(Args ...))
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

		/*NSTD_MAKE_ADDRESS_PIPE(rel8);
		NSTD_MAKE_ADDRESS_PIPE(rel32);*/

		NSTD_MAKE_ADDRESS_PIPE(jmp);

#undef NSTD_MAKE_ADDRESS_PIPE

		struct invoke_tag
		{
		};

		_INLINE_VAR constexpr auto invoke = invoke_tag( );

		//-----------------

		template <typename T, typename ...Ts>
		address_pipe_impl(address_pipe_impl<Ts...>&& old, T&& val)->address_pipe_impl<Ts..., std::remove_cvref_t<T>>;

		template <not_address_pipe A, typename ...B>
		address_pipe_impl(A&& a, B&& ...b)->address_pipe_impl<std::remove_cvref_t<A>, std::remove_cvref_t<B>...>;

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

		template <typename A, typename B>
			requires(std::constructible_from<address, A>)
		auto operator|(A&& a, B&& b)
		{
			return address_pipe_impl(address(a), std::forward<B>(b));
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

#endif

	//--

}