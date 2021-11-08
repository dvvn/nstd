#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace std
{
	template <class _Fty>
	class function;
}

namespace nstd::file
{
	using allocator_fn = std::function<uint8_t*(size_t)>;
	void to_memory_impl(const std::wstring_view& path, const allocator_fn& allocator);

	namespace detail
	{
		class copyable_inplace_start
		{
			size_t size_used_;
			uint8_t begin_[1];

		protected:
			copyable_inplace_start( ) = default;

		public:
			allocator_fn make_allocator(size_t max_size);

			uint8_t* begin( );
			uint8_t* end( );

			const uint8_t* begin( ) const;
			const uint8_t* end( ) const;

			size_t size( ) const { return size_used_; }
		};
	}

	template <size_t Size>
	class copyable : public detail::copyable_inplace_start
	{
	public:
		copyable( ) = default;
		static constexpr size_t max_size( ) { return Size; }

	private:
		uint8_t tail_[Size - 1];
	};

	template < >
	class copyable<0> : std::vector<uint8_t>
	{
	public:
		using std::vector<uint8_t>::begin;
		using std::vector<uint8_t>::end;
		using std::vector<uint8_t>::size;

		copyable( ) = default;
		allocator_fn make_allocator( );
	};

	template < >
	class copyable<1> : public detail::copyable_inplace_start
	{
	public:
		copyable( ) = default;
		static constexpr size_t max_size( ) { return 1; }
	};

	using copyable_heap = copyable<0>;

	class noncopyable_heap
	{
	public:
		noncopyable_heap( ) = default;

		uint8_t* begin( ) const;
		uint8_t* end( ) const;
		size_t size( ) const;

		allocator_fn make_allocator( );
	private:
		std::unique_ptr<uint8_t[]> buffer_;
		size_t size_ = 0;
	};

	template <typename Out = noncopyable_heap>
	Out to_memory(const std::wstring_view& path)
	{
		Out out;

		if constexpr (std::derived_from<Out, detail::copyable_inplace_start>)
			to_memory_impl(path, out.make_allocator(out.max_size( )));
		else
			to_memory_impl(path, out.make_allocator( ));

		return out;
	}
}
