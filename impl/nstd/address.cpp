#include "address.h"

#include "runtime_assert_fwd.h"

using namespace nstd;

address::address()
	: value_(0)
{
}

address::address(uintptr_t a)
	: value_(a)
{
}

address::address(std::nullptr_t)
	: value_(0)
{
}

address::address(const void* a)
	: ptr_((a))
{
}

address::address(void* a)
	: ptr_((a))
{
}

uintptr_t address::value() const
{
	return value_;
}

address address::deref(ptrdiff_t count) const
{
	runtime_assert(count != 0, "Count must be not zero!");

	auto result = *this;
	while (count-- > 0)
		result = *result;
	return result;
}

address address::deref_safe(ptrdiff_t count) const
{
	return count == 0 ? *this : deref(count);
}

address address::add(ptrdiff_t offset) const
{
	return *this + offset;
}

address address::remove(ptrdiff_t offset) const
{
	return *this - offset;
}

address address::multiply(ptrdiff_t value) const
{
	return *this * value;
}

address address::divide(ptrdiff_t value) const
{
	return *this / value;
}

address address::rel8(ptrdiff_t offset) const
{
	const auto out = *this + offset;

	// get relative offset.
	const auto r = out.ref<uint8_t>( );
	runtime_assert(r != 0, "can't get rel8 offset");
	/*if (!r)
		return t{ };*/

	// relative to address of next instruction.
	// short jumps can go std::forward and backward depending on the size of the second byte.
	// if the second byte is below 128, the jmp goes forwards.
	// if the second byte is above 128, the jmp goes backwards ( subtract two's complement of the relative offset from the address of the next instruction ).
	if (r < 128)
		return (out + 1) + r;
	else
		return (out + 1) - static_cast<uint8_t>(~r + 1);
}

address address::rel32(ptrdiff_t offset) const
{
	const auto out = *this + offset;

	// get rel32 offset.
	const auto r = out.ref<uint32_t>( );
	runtime_assert(r != 0, "can't get rel32 offset");

	// relative to address of next instruction.
	return (out + 4) + r;
}

address address::jmp(ptrdiff_t offset) const
{
	// Example:
	// E9 ? ? ? ?
	// The offset has to skip the E9 (JMP) instruction
	// Then deref the address coming after that to get to the function
	// Since the relative JMP is based on the next instruction after the address it has to be skipped

	// Base address is the address that follows JMP ( 0xE9 ) instruction
	auto base = this->value_ + offset;

	// Store the displacement
	// Note: Displacement addresses can be signed, thanks d3x
	auto displacement = *reinterpret_cast<int32_t*>(base);

	// The JMP is based on the instruction after the address
	// so the address size has to be added
	// Note: This is always 4 bytes, regardless of architecture, thanks d3x
	base += sizeof(uint32_t);

	/// Now finally do the JMP by adding the function address
	base += displacement;

	return (base);
}

//----------------

bool nstd::operator==(address l, address r)
{
	return l.value( ) == r.value( );
}

bool nstd::operator!=(address l, address r)
{
	return !(l == r);
}

bool nstd::operator<(address l, address r)
{
	return l.value( ) < r.value( );
}

bool nstd::operator>(address l, address r)
{
	return l.value( ) > r.value( );
}

bool nstd::operator<=(address l, address r)
{
	return l.value( ) <= r.value( );
}

bool nstd::operator>=(address l, address r)
{
	return l.value( ) >= r.value( );
}

//-------------

#define NSTD_ADDRESS_OPERATOR_CALL(...) \
	decltype(auto) temp = __VA_ARGS__;\
	runtime_assert(address(temp).value( ) != 0u, "Address is null!");\
	runtime_assert(address(temp).value( ) != static_cast<uintptr_t>(-1), "Address is incorrect!");\
	return temp;

address nstd::operator+(address l, ptrdiff_t r)
{
	NSTD_ADDRESS_OPERATOR_CALL(l.value( ) + r);
}

address nstd::operator+(ptrdiff_t l, address r)
{
	return r + l;
}

address nstd::operator-(address l, ptrdiff_t r)
{
	NSTD_ADDRESS_OPERATOR_CALL(l.value( ) - r);
}

address nstd::operator-(ptrdiff_t l, address r)
{
	return r - l;
}

address& nstd::operator+=(address& l, ptrdiff_t r)
{
	return l = l + r;
}

address& nstd::operator+=(ptrdiff_t l, address& r)
{
	return r += l;
}

address& nstd::operator-=(address& l, ptrdiff_t r)
{
	return l = l - r;
}

address& nstd::operator-=(ptrdiff_t l, address& r)
{
	return r -= l;
}

//----------------

address nstd::operator*(address l, ptrdiff_t r)
{
	NSTD_ADDRESS_OPERATOR_CALL(l.value( ) * r);
}

address nstd::operator*(ptrdiff_t l, address r)
{
	return r * l;
}

address nstd::operator/(address l, ptrdiff_t r)
{
	NSTD_ADDRESS_OPERATOR_CALL(l.value( ) / r);
}

address nstd::operator/(ptrdiff_t l, address r)
{
	return r / l;
}

address& nstd::operator*=(address& l, ptrdiff_t r)
{
	return l = l * r;
}

address& nstd::operator*=(ptrdiff_t l, address& r)
{
	return r *= l;
}

address& nstd::operator/=(address& l, ptrdiff_t r)
{
	return l = l / r;
}

address& nstd::operator/=(ptrdiff_t l, address& r)
{
	return r /= l;
}

//---

address nstd::operator*(address a)
{
	return a.ref<uintptr_t>( );
}
