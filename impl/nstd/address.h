#pragma once

#include <type_traits>

namespace nstd
{
	class address;
	bool operator ==(address l, address r);
	bool operator !=(address l, address r);
	bool operator <(address l, address r);
	bool operator >(address l, address r);
	bool operator <=(address l, address r);
	bool operator >=(address l, address r);

	address operator +(address l, ptrdiff_t r);
	address operator +(ptrdiff_t l, address r);
	address operator -(address l, ptrdiff_t r);
	address operator -(ptrdiff_t l, address r);

	address& operator +=(address& l, ptrdiff_t r);
	address& operator +=(ptrdiff_t l, address& r);
	address& operator -=(address& l, ptrdiff_t r);
	address& operator -=(ptrdiff_t l, address& r);

	address operator *(address l, ptrdiff_t r);
	address operator *(ptrdiff_t l, address r);
	address operator /(address l, ptrdiff_t r);
	address operator /(ptrdiff_t l, address r);

	address& operator *=(address& l, ptrdiff_t r);
	address& operator *=(ptrdiff_t l, address& r);
	address& operator /=(address& l, ptrdiff_t r);
	address& operator /=(ptrdiff_t l, address& r);

	address operator*(address a);

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

	// class size is only 4 bytes on x86-32 and 8 bytes on x86-64.
	class address
	{
	public:
		address( );
		explicit address(uintptr_t a);
		address(std::nullptr_t);
		address(const void* a);
		address(void* a);

		uintptr_t value( ) const;

		template <typename T>
		T cast( ) const
		{
#ifdef _DEBUG
			(*this) + 0u;
#endif

			if constexpr (reinterpret_convertible_to<uintptr_t, T>)
			{
				return reinterpret_cast<T>(value_);
			}
			else if constexpr (std::is_member_pointer_v<T>)
			{
				T ret;
				reinterpret_cast<decltype(ptr_)&>(ret) = ptr_;
				return ret;
			}
			else
			{
				static_assert(std::_Always_false<T>, __FUNCTION__": unable to cast");
				return T( );
			}
		}

		class ptr_auto_cast
		{
			uintptr_t addr_;
		public:
			ptr_auto_cast(uintptr_t addr)
				: addr_(addr)
			{
			}

			template <typename T>
			operator T*( ) const { return reinterpret_cast<T*>(addr_); }
		};

		template <typename T = ptr_auto_cast>
		auto ptr( ) const
		{
			if constexpr (std::is_same_v<T, ptr_auto_cast>)
				return ptr_auto_cast(value_);
			else
				return cast<T*>( );
		}

		class ref_auto_cast
		{
			uintptr_t addr_;
		public:
			ref_auto_cast(uintptr_t addr)
				: addr_(addr)
			{
			}

			template <typename T>
			operator T&( ) const { return *reinterpret_cast<T*>(addr_); }
		};

		template <typename T = ref_auto_cast>
		decltype(auto) ref( ) const
		{
			if constexpr (std::is_same_v<T, ref_auto_cast>)
				return ref_auto_cast(value_);
			else
				return *ptr<T>( );
		}

		//-----

		//derefference
		address deref(ptrdiff_t count) const;
		[[deprecated]]
		address deref_safe(ptrdiff_t count) const;

		address add(ptrdiff_t offset) const;
		address remove(ptrdiff_t offset) const;
		address multiply(ptrdiff_t value) const;
		address divide(ptrdiff_t value) const;

		// follow relative8 and relative16/32 offsets.
		address rel8(ptrdiff_t offset) const;
		address rel32(ptrdiff_t offset) const;

		address jmp(ptrdiff_t offset = 0x1) const;

	private:
		union
		{
			// ReSharper disable CppInconsistentNaming
			uintptr_t value_;
			const void* ptr_;
			// ReSharper restore CppInconsistentNaming
		};
	};
}
