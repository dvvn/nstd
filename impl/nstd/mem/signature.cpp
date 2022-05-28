module;

#include <nstd/ranges.h>
#include <nstd/runtime_assert.h>

#include <memory>
#include <span>
#include <string_view>
#include <vector>

module nstd.mem.signature;

template <class Rng, class T>
struct abstract_storage : T
{
    using buffer_type = Rng;

    abstract_storage() = default;

    abstract_storage(buffer_type&& buffer)
        : buffer_(std::move(buffer))
    {
    }

    T::pointer begin() const final
    {
        return buffer_.data();
    }

    T::pointer end() const final
    {
        return buffer_.data() + buffer_.size();
    }

    size_t size() const final
    {
        return buffer_.size();
    }

    buffer_type* get()
    {
        return &buffer_;
    }

    const buffer_type* get() const
    {
        return &buffer_;
    }

  private:
    buffer_type buffer_;
};

using known_bytes_internal = abstract_storage<std::vector<uint8_t>, known_bytes>;
using known_bytes_external = abstract_storage<std::span<const uint8_t>, known_bytes>;

template <typename T>
static void _Store_bytes_to(const uint8_t* begin, const uint8_t* end, std::unique_ptr<known_bytes>& data)
{
    runtime_assert(data == nullptr);
    data = std::make_unique<T>(T::buffer_type(begin, end));
}

//---------------

auto unknown_bytes::operator[](const size_t index) const -> const value_type&
{
    return *(begin() + index);
}

size_t unknown_bytes::bytes_count() const
{
    size_t ret = 0;
    for (auto& [buff, skip] : *this)
    {
        ret += buff->size();
        ret += skip;
    }

    return ret;
}

struct unknown_bytes_impl : abstract_storage<std::vector<unknown_bytes_data>, unknown_bytes>
{
    void push_back(value_type&& val) final
    {
        this->get()->push_back(std::move(val));
    }
};

//---------------

struct unknown_bytes_data_dynamic
{
    known_bytes_internal buff_;
    uint16_t skip_ = 0;

  public:
    unknown_bytes_data_dynamic() = default;

    operator unknown_bytes_data() &&
    {
        unknown_bytes_data out;
        static_assert(std::same_as<decltype(skip_), decltype(out.skip)>);
        out.buff = std::make_unique<known_bytes_internal>(std::move(buff_));
        out.skip = skip_;
        return out;
    }

    void reset()
    {
        buff_.get()->clear();
        skip_ = 0;
    }

    bool can_skip() const
    {
        return skip_ > 0;
    }

    void skip(const uint8_t step = 1)
    {
        skip_ += step;
    }

    bool empty() const
    {
        return buff_.get()->empty() && skip_ == 0;
    }

    void push_back(const uint8_t byte)
    {
        buff_.get()->push_back(byte);
    }
};

class unknown_bytes_writer
{
    unknown_bytes_impl source_;
    unknown_bytes_data_dynamic target_;

    void dump_impl()
    {
        source_.push_back(std::move(target_));
    }

  public:
    void move_to(std::unique_ptr<unknown_bytes>& data)
    {
        runtime_assert(data == nullptr);
        if (!target_.empty())
            dump_impl();
        data = std::make_unique<unknown_bytes_impl>(std::move(source_));
    }

    void write(const std::string_view rng)
    {
        // write previous part
        if (target_.can_skip())
        {
            dump_impl();
            target_.reset();
        }

        constexpr auto to_num = []<typename T>(T chr) -> uint8_t {
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
                runtime_assert_unreachable("Unsupported character");
            }
        };

        switch (rng.size())
        {
        case 1:
            target_.push_back(to_num(rng[0]));
            break;
        case 2:
            target_.push_back(to_num(rng[0]) * 16 + to_num(rng[1]));
            break;
        default:
            runtime_assert_unreachable("Incorrect string validation!");
        }
    }

    void skip()
    {
        target_.skip();
    }
};

static bool _Validate_signature(const std::string_view rng)
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
        case 'F': {
            if (qmarks > 0)
                return false;
            ++counter;
            if (counter > 2)
                return false;
            qmarks = spaces = 0;
            break;
        }
        case '?': {
            ++qmarks;
            if (qmarks > 2)
                return false;
            counter = spaces = 0;
            break;
        }
        case ' ': {
            ++spaces;
            if (spaces > 1)
                return false;
            counter = qmarks = 0;
            break;
        }
        default: {
            return false;
        }
        }
    }

    return true;
}

static auto _Text_to_bytes(const char* begin, const char* end)
{
    const std::string_view text_src = {begin, end};
    runtime_assert(_Validate_signature(text_src));

    unknown_bytes_writer writer;

    constexpr auto unwrap_shit = []<class T>(T rng) -> std::string_view {
        const auto raw_begin = std::addressof(*rng.begin());
        const size_t size = nstd::ranges::distance(rng);
        return {raw_begin, size};
    };

    for (const auto b : nstd::views::split(text_src, ' ') | nstd::views::transform(unwrap_shit))
    {
        if (b[0] == '?')
            writer.skip();
        else
            writer.write(b);
    }

    return writer;
}

//----

known_signature::known_signature(const uint8_t* begin, const size_t mem_size)
{
    _Store_bytes_to<known_bytes_external>(begin, begin + mem_size, *this);
}

unknown_signature::unknown_signature(const std::string_view str)
{
    _Text_to_bytes(str.data(), str.data() + str.size()).move_to(*this);
}

unknown_signature::unknown_signature(const char* begin, const size_t mem_size)
{
    _Text_to_bytes(begin, begin + mem_size).move_to(*this);
}
