module;

#include "address_includes.h"

module nstd.mem:address;

using namespace nstd::mem;

uintptr_t address::value( ) const
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
	return *this + address(offset);
}

address& address::add_self(ptrdiff_t offset)
{
	return *this += address(offset);
}

address address::remove(ptrdiff_t offset) const
{
	return *this - address(offset);
}

address address::multiply(ptrdiff_t value) const
{
	return *this * address(offset);
}

address address::divide(ptrdiff_t value) const
{
	return *this / address(offset);
}

#if 0
address address::rel8(ptrdiff_t offset) const
{
	const auto out = *this + offset;

	// get relative offset.
	const uint8_t r = out.ref( );
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
	const uint32_t r = out.ref( );
	runtime_assert(r != 0, "can't get rel32 offset");

	// relative to address of next instruction.
	return (out + 4) + r;
}
#endif
address address::jmp(ptrdiff_t offset) const
{
	//same as rel 32

	// Example:
	// E9 ? ? ? ?
	// The offset has to skip the E9 (JMP) instruction
	// Then deref the address coming after that to get to the function
	// Since the relative JMP is based on the next instruction after the address it has to be skipped

	// Base address is the address that follows JMP ( 0xE9 ) instruction
	auto base = this->add(offset);

	// Store the displacement
	// Note: Displacement addresses can be signed
	int32_t displacement = base.ref( );

	// The JMP is based on the instruction after the address
	// so the address size has to be added
	// Note: This is always 4 bytes, regardless of architecture
	base.add_self(sizeof(uint32_t));

	/// Now finally do the JMP by adding the function address
	base.add_self(displacement);

	return base;
}

//----------------

std::strong_ordering address::operator<=>(address other)const
{
	return this->value_ <=> other.value_;
}

#define NSTD_ADDRESS_VALIDATE_IMPL(_SOURCE_)\
runtime_assert(_SOURCE_ != static_cast<uintptr_t>(0), "Address is null!");\
runtime_assert(_SOURCE_ != static_cast<uintptr_t>(-1), "Address is incorrect!");
#define NSTD_ADDRESS_VALIDATE\
NSTD_ADDRESS_VALIDATE_IMPL(this->value_);\
NSTD_ADDRESS_VALIDATE_IMPL(other.value_);

#define NSTD_ADDRESS_OPERATOR(_OP_)\
address address::operator##_OP_##(address other)const\
{\
	NSTD_ADDRESS_VALIDATE;\
	return {this->value_ _OP_ other.value_, address_value_construct( )};\
}\
address& address::operator##_OP_##= (address other)\
{\
	NSTD_ADDRESS_VALIDATE;\
	this->value_ _OP_##= other.value_;\
	return *this;\
}

NSTD_ADDRESS_OPERATOR(+);
NSTD_ADDRESS_OPERATOR(-);
NSTD_ADDRESS_OPERATOR(*);
NSTD_ADDRESS_OPERATOR(/ );

address address::operator*( )
{
	return this->ref( );
}
