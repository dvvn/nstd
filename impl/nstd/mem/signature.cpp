module;

#include <nstd/runtime_assert.h>
#include <nstd/ranges.h>

#include <variant>
#include <vector>
#include <string_view>
#include <span>

module nstd.mem.signature;

known_bytes::known_bytes( ) = default;

known_bytes::known_bytes(buffer_type && data)
	:data_(std::move(data))
{
}
known_bytes::known_bytes(const range_type data)
	: data_(data)
{
}

auto known_bytes::begin( ) const noexcept -> pointer
{
	return std::visit([]<class T>(const T & mem) noexcept
	{
		return mem.data( );
	}, data_);
}

auto known_bytes::end( ) const noexcept -> pointer
{
	return std::visit([]<class T>(const T & mem) noexcept
	{
		return mem.data( ) + mem.size( );
	}, data_);
}

size_t known_bytes::size( )const noexcept
{
	return std::visit([]<class T>(const T & mem) noexcept
	{
		return mem.size( );
	}, data_);
}

//---------------

void unknown_bytes::push_back(unknown_bytes_data && val) noexcept
{
	data_.push_back(std::move(val));
}

auto unknown_bytes::begin( ) const noexcept -> pointer
{
	return data_.data( );
}

auto unknown_bytes::end( ) const noexcept -> pointer
{
	return data_.data( ) + data_.size( );
}

size_t unknown_bytes::size( ) const noexcept
{
	return data_.size( );
}

const unknown_bytes_data& unknown_bytes::operator[](const size_t index) const noexcept
{
	return data_[index];
}

size_t unknown_bytes::bytes_count( ) const noexcept
{
	size_t ret = 0;
	for (auto& [buff, skip] : data_)
	{
		ret += buff.size( );
		ret += skip;
	}

	return ret;
}

//---------------

static bool validate_signature(const std::string_view rng) noexcept
{
	if (rng.starts_with('?'))
		return false;
	if (rng.starts_with(' '))
		return false;
	if (rng.ends_with(' '))
		return false;

	uint8_t qmarks = 0;
	uint8_t spaces = 0;
	uint8_t counter = 0;
	for (const auto c : rng)
	{
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		{
			if (qmarks > 0)
				return false;
			++counter;
			if (counter > 2)
				return false;
			qmarks = spaces = 0;
			break;
		}
		case '?':
		{
			++qmarks;
			if (qmarks > 2)
				return false;
			counter = spaces = 0;
			break;
		}
		case ' ':
		{
			++spaces;
			if (spaces > 1)
				return false;
			counter = qmarks = 0;
			break;
		}
		default:
		{
			return false;
		}
		}
	}

	return true;
}

class unknown_bytes_writer
{
	unknown_bytes source_;
	known_bytes::buffer_type buff_;
	unknown_bytes_data::skip_type skip_ = 0;

	template<bool Move>
	void dump_impl( ) noexcept(Move)
	{
		unknown_bytes_data tmp;
		known_bytes::buffer_type buff;
		if constexpr (Move)
			buff = std::move(buff_);
		else
			buff = buff_;

		tmp.buff = std::move(buff);
		tmp.skip = skip_;

		source_.push_back(std::move(tmp));
	}

	void reset( )noexcept
	{
		buff_.clear( );
		skip_ = 0;
	}

public:
	operator unknown_bytes( ) && noexcept
	{
		if (!buff_.empty( ) || skip_ > 0)
			dump_impl<true>( );
		return std::move(source_);
	}

	void write(const std::string_view rng) noexcept
	{
		//write previous part
		if (skip_ > 0)
		{
			dump_impl<true>( );
			reset( );
		}

		constexpr auto to_num = []<typename T>(T chr) noexcept
		{
			switch (chr)
			{
			case '0':
				return 0x0;
			case '1':
				return 0x1;
			case '2':
				return 0x2;
			case '3':
				return 0x3;
			case '4':
				return 0x4;
			case '5':
				return 0x5;
			case '6':
				return 0x6;
			case '7':
				return 0x7;
			case '8':
				return 0x8;
			case '9':
				return 0x9;
			case 'a':
			case 'A':
				return 0xA;
			case 'b':
			case 'B':
				return 0xB;
			case 'c':
			case 'C':
				return 0xC;
			case 'd':
			case 'D':
				return 0xD;
			case 'e':
			case 'E':
				return 0xE;
			case 'f':
			case 'F':
				return 0xF;
			default:
				runtime_assert("Unsupported character");
			}
		};

		switch (rng.size( ))
		{
		case 1:
			buff_.push_back(to_num(rng[0]));
			break;
		case 2:
			buff_.push_back(to_num(rng[0]) * 16 + to_num(rng[1]));
			break;
		default:
			runtime_assert("Incorrect string validation!");
		}
	}

	void skip(size_t step = 1) noexcept
	{
		++skip_;
	}
};

static unknown_bytes text_to_bytes(const char* begin, const char* end) noexcept
{
	runtime_assert(validate_signature({begin,end}));

	unknown_bytes_writer writer;

	constexpr auto unwrap_shit = []<class T>(T rng) noexcept
	{
		return std::string_view(&*rng.begin( ), ranges::distance(rng));
	};

	for (const auto b : std::views::split(std::span(begin, end), ' ') | std::views::transform(unwrap_shit))
	{
		if (b[0] == '?')
			writer.skip( );
		else
			writer.write(b);
	}

	return std::move(writer);
}

//----

using namespace nstd;

auto mem::make_signature(const std::string_view str) noexcept -> signature_unknown_bytes
{
	return text_to_bytes(str.data( ), str.data( ) + str.size( ));
}

auto mem::make_signature(const char* begin, const size_t mem_size) noexcept -> signature_unknown_bytes
{
	return text_to_bytes(begin, begin + mem_size);
}

auto mem::make_signature_known(const uint8_t * begin, const size_t mem_size) noexcept -> signature_known_bytes
{
	const known_bytes::range_type mem = {begin,begin + mem_size};
	return mem;
}