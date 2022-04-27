module;

#include <nstd/core.h>

#include <concepts>

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
Out _Force_cast(In in) noexcept
{
	if constexpr(reinterpret_convertible_to<In, Out>)
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

template<typename T>
decltype(auto) _Deref(T* const ptr) noexcept
{
	if constexpr(std::is_pointer_v<T>)
		return *ptr;
	else
		return *_Force_cast<std::conditional_t<std::is_class_v<T> || std::is_member_function_pointer_v<T> || std::is_function_v<T>, void, T>**>(ptr);
}

template<size_t Count, typename T>
decltype(auto) _Deref(T* const ptr) noexcept
{
	const auto ptr1 = _Deref(ptr);
	if constexpr(Count == 1)
		return ptr1;
	else
		return _Deref<Count - 1>(ptr1);
}

template<typename From, typename T>
concept constructible_from = std::constructible_from<T, From>;

template<typename T>
constexpr size_t _Array_step( )noexcept
{
	if constexpr(std::is_void_v<T>)
		return sizeof(uintptr_t);
	else
		return sizeof(T);
}

export namespace nstd::mem
{
	template<typename T>
	struct basic_address
	{
		using value_type = /*std::remove_const_t<T>*/T;
		using pointer_type = std::add_pointer_t<T>;
		using safe_out_type = std::conditional_t<std::is_class_v<T>, void, T>;

		union
		{
			uintptr_t value;
			pointer_type pointer;
		};

		basic_address( )
			:pointer(nullptr)
		{
		}

		basic_address(std::nullptr_t)
			:value(0)
		{
		}

		template<std::integral Q>
		basic_address(const Q val)
			: value(static_cast<uintptr_t>(val))
		{
		}

		template<typename Q>
		basic_address(Q* ptr)
			: pointer(_Force_cast<pointer_type>(ptr))
		{
			static_assert(!std::is_class_v<value_type> || std::convertible_to<pointer_type, Q*>, __FUNCSIG__": unable to construct from pointer!");
		}

		template<typename Q>
		basic_address(const basic_address<Q> other)
			: basic_address(other.pointer)
		{
		}

		//----

		bool operator!( ) const noexcept
		{
			return this->pointer == nullptr;
		}

		explicit operator bool( ) const noexcept
		{
			return this->pointer != nullptr;
		}

		//----

#define ADDR_EQ_OP(_OP_)\
		template<constructible_from<basic_address> Q>\
		auto NSTD_CONCAT(operator,_OP_)(const Q other) const noexcept\
		{\
			return this->value _OP_ basic_address(other).value;\
		}

#define ADDR_MATH_OP(_OP_,_NAME_)\
		template<constructible_from<basic_address> Q>\
		basic_address& NSTD_CONCAT(operator,NSTD_CONCAT(_OP_,=))(const Q other) noexcept\
		{\
			static_assert(!std::is_class_v<value_type>, __FUNCSIG__": unable to change the class type!");\
			this->value NSTD_CONCAT(_OP_,=) basic_address<void>(other).value;\
			return *this;\
		}\
		template<constructible_from<basic_address> Q>\
		basic_address<safe_out_type> NSTD_CONCAT(operator,_OP_)(const Q other) const noexcept\
		{\
			return this->value _OP_ basic_address<void>(other).value;\
		}\
		template<constructible_from<basic_address> Q>\
		basic_address<safe_out_type> _NAME_(const Q other) const noexcept\
		{\
			return this->value _OP_ basic_address<void>(other).value;\
		}

		ADDR_EQ_OP(<=> );
		ADDR_EQ_OP(== );

		ADDR_MATH_OP(+, plus);
		ADDR_MATH_OP(-, minus);
		ADDR_MATH_OP(*, multiply);
		ADDR_MATH_OP(/ , divide);

		//----

		auto operator[](const ptrdiff_t index) const noexcept
		{
			auto tmp = *this;
			tmp.value += index * _Array_step<value_type>( );
			return tmp.deref<1>( );
		}

		pointer_type operator->( ) const noexcept
		{
			return this->pointer;
		}

		//----

		template <typename Q>
			requires(std::is_reference_v<Q>)
		operator Q( ) const noexcept
		{
			using ref_t = std::remove_reference_t<Q>;
			static_assert(!std::is_class_v<ref_t> || std::convertible_to<value_type, ref_t>);
			return *_Force_cast<ref_t*>(this->value);
		}

		template <typename Q>
			requires(std::is_pointer_v<Q> || std::is_member_function_pointer_v<Q> || std::is_function_v<Q>)
		operator Q( ) const noexcept
		{
			if constexpr(std::is_pointer_v<Q>)
				static_assert(std::constructible_from<basic_address, Q>, __FUNCSIG__": unable to convert to pointer!");
			else
				static_assert(!std::is_class_v<value_type>, __FUNCSIG__": unable to convert to function pointer!");
			return _Force_cast<Q>(this->value);
		}

		template <typename Q>
			requires(std::is_integral_v<Q>)
		/*explicit*/ operator Q( ) const noexcept
		{
			return static_cast<Q>(this->value);
		}

		//----

		template<size_t Count>
		auto deref( ) const noexcept
		{
			const auto ptr = _Deref<Count>(this->pointer);
			return basic_address<decltype(ptr)>(ptr);
		}

		template<typename Q>
		/*[[deprecated]]*/ Q get( ) const noexcept
		{
			return _Force_cast<Q>(value);
		}

		//----

		basic_address<safe_out_type> jmp(const ptrdiff_t offset) const noexcept
		{
			// Example:
			// E9 ? ? ? ?
			// The offset has to skip the E9 (JMP) instruction
			// Then deref the address coming after that to get to the function
			// Since the relative JMP is based on the next instruction after the basic_address it has to be skipped

			// Base address is the address that follows JMP ( 0xE9 ) instruction
			basic_address<void> base = this->value + offset;

			// Store the displacement
			// Note: Displacement address can be signed
			int32_t displacement = base.deref<1>( );

			// The JMP is based on the instruction after the basic_address
			// so the basic_address size has to be added
			// Note: This is always 4 bytes, regardless of architecture
			base += sizeof(uint32_t);

			// Now finally do the JMP by adding the function basic_address
			base += displacement;

			return base;
		}
	};

	template<typename T>
	basic_address(T*)->basic_address<T>;
}