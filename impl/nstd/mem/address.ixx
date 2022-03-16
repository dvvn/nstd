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
	/*else if constexpr (reinterpret_convertible_to<In, uintptr_t>)
	{
		auto tmp = reinterpret_cast<uintptr_t>(in);
		return reinterpret_cast<Out>(tmp);
	}*/
	else
	{
		return reinterpret_cast<Out&>(reinterpret_cast<uintptr_t&>(in));
	}
}

export namespace nstd::inline mem
{
	struct address_tag { };

	template<class T>
	concept have_address_tag = std::derived_from<T, address_tag>;

	template<typename T>
	struct basic_address : address_tag
	{
		// class size is only 4 bytes on x86-32 and 8 bytes on x86-64.

		using value_type = /*std::remove_const_t<T>*/T;
		using pointer_type = std::add_pointer_t<T>;

		union
		{
			uintptr_t value;
			pointer_type pointer;
		};

		basic_address( )
			:value(0)
		{
		}
		explicit basic_address(uintptr_t val)
			: value(val)
		{
		}
		basic_address(std::nullptr_t)
			: pointer(nullptr)
		{
		}
		basic_address(pointer_type ptr)
			: pointer(ptr)
		{
		}

		template<typename Q>
		basic_address(Q* ptr)
			: pointer(force_cast<pointer_type>(ptr))
		{
		}


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

		template<size_t Count>
		basic_address deref( )const
		{
			const basic_address addr = *force_cast<pointer_type*>(pointer);
			if constexpr (Count == 1)
				return addr;
			else
				return addr.deref<Count - 1>( );
		}

		[[deprecated]]
		basic_address deref(size_t count) const
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

		/*ptr_auto_cast<uintptr_t> ptr( ) const
		{
			return value;
		}*/

		template <typename Q>
		auto& ref( ) const
		{
			return *ptr<Q>( );
		}

		ref_auto_cast<uintptr_t> ref( ) const
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

		basic_address operator[](ptrdiff_t index) const;

		basic_address<void> jmp(ptrdiff_t offset = 0x1) const;
		basic_address<void> plus(ptrdiff_t offset)const;
		basic_address<void> minus(ptrdiff_t offset)const;
		basic_address<void> multiply(ptrdiff_t value)const;
		basic_address<void> divide(ptrdiff_t value)const;
	};

	template<typename T>
	basic_address(T*)->basic_address</*std::remove_cv_t<T>*/T>;

	using address = basic_address<void>;

	//---

	template<typename T, typename Addr>
	concept address_constructible = std::constructible_from<Addr, T>;

	template<typename T>
	auto& _Unwrap_address_value(T& obj)
	{
		if constexpr (have_address_tag<std::remove_cv_t<T>>)
			return obj.value;
		else
			return reinterpret_cast<basic_address<void>&>(obj).value;
	}

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

	NSTD_ADDRESS_OPERATOR_MATH(+);
	NSTD_ADDRESS_OPERATOR_MATH(-);
	NSTD_ADDRESS_OPERATOR_MATH(*);
	NSTD_ADDRESS_OPERATOR_MATH(/ );
	NSTD_ADDRESS_OPERATOR_EQUALITY(< );
	NSTD_ADDRESS_OPERATOR_EQUALITY(<= );
	NSTD_ADDRESS_OPERATOR_EQUALITY(> );
	NSTD_ADDRESS_OPERATOR_EQUALITY(>= );
	NSTD_ADDRESS_OPERATOR_EQUALITY(== );

	template<typename T>
	basic_address<T> basic_address<T>::operator[](ptrdiff_t index) const
	{
		constexpr auto step = []
		{
			if constexpr (std::is_void_v<T>)
				return sizeof(uintptr_t);
			else
				return sizeof(T);
		}();

		const auto element = *this + index * step;
		return *element;
	}


	template<typename T>
	basic_address<void> basic_address<T>::jmp(ptrdiff_t offset) const
	{
		//same as rel 32

		// Example:
		// E9 ? ? ? ?
		// The offset has to skip the E9 (JMP) instruction
		// Then deref the address coming after that to get to the function
		// Since the relative JMP is based on the next instruction after the basic_address it has to be skipped

		// Base address is the address that follows JMP ( 0xE9 ) instruction
		basic_address<void> base = *this + offset;

		// Store the displacement
		// Note: Displacement address can be signed
		int32_t displacement = *base;

		// The JMP is based on the instruction after the basic_address
		// so the basic_address size has to be added
		// Note: This is always 4 bytes, regardless of architecture
		base += sizeof(uint32_t);

		// Now finally do the JMP by adding the function basic_address
		base += displacement;

		return base;
	}

	template<typename T>
	basic_address<void> basic_address<T>::plus(ptrdiff_t offset)const
	{
		return *this + offset;
	}

	template<typename T>
	basic_address<void> basic_address<T>::minus(ptrdiff_t offset)const
	{
		return *this - offset;
	}

	template<typename T>
	basic_address<void> basic_address<T>::multiply(ptrdiff_t val)const
	{
		return *this * val;
	}

	template<typename T>
	basic_address<void> basic_address<T>::divide(ptrdiff_t val)const
	{
		return *this / val;
	}
}