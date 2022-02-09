#include "to_memory.h"

#include <nstd/runtime_assert.h>

#include <fstream>
#include <functional>
#include <filesystem>

using namespace nstd;
using namespace file;
using namespace file::detail;

void file::to_memory_impl(const std::wstring_view& path, const allocator_fn& allocator)
{
	using namespace std;

	ifstream infile;
	constexpr auto open_flags = ios::in | ios::binary | ios::ate;
	if (*(path.data()+path.size()) == '\0')
		infile.open(path.data( ), open_flags);
	else
		infile.open(std::filesystem::path(path), open_flags);

	//infile.seekg(0, infile.end); //done by ios::ate
	const make_unsigned_t<streamoff> file_size0 = infile.tellg( );
	runtime_assert(file_size0 > 0 && file_size0 < numeric_limits<size_t>::max());
	const auto file_size = static_cast<size_t>(file_size0);

	infile.seekg(0, ios::beg);

	const auto buffer = allocator(file_size);
	infile.read(reinterpret_cast<char*>(buffer), file_size);

	runtime_assert(!infile.bad());
}

allocator_fn copyable_inplace_start::make_allocator(size_t max_size)
{
	return [this, max_size](size_t file_size)
	{
		runtime_assert(file_size <= max_size);
		size_used_ = file_size;
		return begin( );
	};
}

uint8_t* copyable_inplace_start::begin( )
{
	return begin_;
}

uint8_t* copyable_inplace_start::end( )
{
	return begin_ + size_used_;
}

const uint8_t* copyable_inplace_start::begin( ) const
{
    return const_cast<copyable_inplace_start*>(this)->begin();
}

const uint8_t* copyable_inplace_start::end( ) const
{
    return const_cast<copyable_inplace_start*>(this)->end();
}

size_t copyable_inplace_start::size( ) const
{
	return size_used_; 
}

allocator_fn copyable<0>::make_allocator( )
{
	return [&](size_t file_size)
	{
		runtime_assert(this->empty());
		this->resize(file_size);
		return this->data( );
	};
}

uint8_t* noncopyable_heap::begin( ) const
{
	return buffer_.get( );
}

uint8_t* noncopyable_heap::end( ) const
{
	return buffer_.get( ) + size_;
}

size_t noncopyable_heap::size( ) const
{
	return size_;
}

allocator_fn noncopyable_heap::make_allocator( )
{
	return [&](size_t file_size)
	{
		buffer_ = std::make_unique<uint8_t[]>(file_size);
		size_   = file_size;
		return buffer_.get( );
	};
}
