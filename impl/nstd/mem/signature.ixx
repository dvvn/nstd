module;

#include <memory>
#include <span>
#include <string_view>

export module nstd.mem.signature;

/* class known_bytes
{
public:
    using buffer_type = std::vector<uint8_t>;
    using range_type = std::span<const uint8_t>;

    known_bytes();

    known_bytes(buffer_type&& data);
    known_bytes(const range_type data);

    using pointer = range_type::pointer;

    pointer begin() const;
    pointer end() const;
    size_t size() const;

  private:
    std::variant<buffer_type, range_type> data_;
}; */

struct known_bytes
{
    using pointer = const uint8_t*;

    virtual ~known_bytes() = default;

    virtual pointer begin() const = 0;
    virtual pointer end() const = 0;
    virtual size_t size() const = 0;
};

struct unknown_bytes_data
{
    std::unique_ptr<known_bytes> buff;
    // unknown bytes after buffer
    uint16_t skip = 0;
    //?? ?? ? AA 11 22 BB ?? ?? ? CC 11 0F
    //[  1  ] [         2       ] [  3  ]
    // 1) buff(empty), skip==3
    // 2) buff(size==4) skip==3
    // 3) buff(size==3) skip(empty)
};

/* class unknown_bytes
{
public:
    using pointer = const unknown_bytes_data*;
    using value_type = unknown_bytes_data;

    void push_back(unknown_bytes_data&& val);

    pointer begin() const;
    pointer end() const;
    size_t size() const;

    const unknown_bytes_data& operator[](const size_t index) const;
    size_t bytes_count() const;

  private:
    std::vector<unknown_bytes_data> data_;
}; */

struct unknown_bytes
{
    using pointer = const unknown_bytes_data*;
    using value_type = unknown_bytes_data;

    virtual void push_back(value_type&& val) = 0;

    virtual pointer begin() const = 0;
    virtual pointer end() const = 0;
    virtual size_t size() const = 0;

    const value_type& operator[](const size_t index) const;
    size_t bytes_count() const;
};

struct known_signature : std::unique_ptr<known_bytes>
{
    known_signature(const uint8_t* begin, const size_t mem_size);
};

struct unknown_signature : std::unique_ptr<unknown_bytes>
{
    unknown_signature(const std::string_view str);
    unknown_signature(const char* begin, const size_t mem_size);
};

export namespace nstd::mem
{
    using ::known_signature;
    using ::unknown_signature;
} // namespace nstd::mem
