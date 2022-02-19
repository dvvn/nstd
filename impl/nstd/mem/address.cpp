module;

#include "address_includes.h"

module nstd.mem:address;

using namespace nstd::mem;

#if 0

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

address address::operator*( )
{
	return this->ref( );
}
#endif

#define NSTD_ADDRESS_PROVIDE(_TYPE_)\
template struct basic_address<_TYPE_>;/*\
template struct basic_address<const _TYPE_>;*/

NSTD_ADDRESS_PROVIDE(void);
NSTD_ADDRESS_PROVIDE(char);
#ifdef __cpp_lib_char8_t
NSTD_ADDRESS_PROVIDE(char8_t);
#endif
NSTD_ADDRESS_PROVIDE(char16_t);
NSTD_ADDRESS_PROVIDE(char32_t);
NSTD_ADDRESS_PROVIDE(wchar_t);
NSTD_ADDRESS_PROVIDE(int8_t);
NSTD_ADDRESS_PROVIDE(uint8_t);
NSTD_ADDRESS_PROVIDE(int16_t);
NSTD_ADDRESS_PROVIDE(uint16_t);
NSTD_ADDRESS_PROVIDE(int32_t);
NSTD_ADDRESS_PROVIDE(uint32_t);
NSTD_ADDRESS_PROVIDE(int64_t);
NSTD_ADDRESS_PROVIDE(uint64_t);