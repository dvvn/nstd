module;

#include <nstd/runtime_assert_core.h>

#include <variant>
#include <vector>
#include <string_view>
#include <span>

export module nstd.mem.signature;

class known_bytes
{
public:
	using buffer_type = std::vector<uint8_t>;
	using range_type = std::span<const uint8_t>;

	known_bytes();

	known_bytes(buffer_type&& data);
	known_bytes(const range_type data);

	using pointer = range_type::pointer;

	pointer begin() const noexcept;
	pointer end() const noexcept;
	size_t size() const noexcept;

private:
	std::variant<buffer_type, range_type> data_;
};

struct unknown_bytes_data
{
	using skip_type = uint16_t;

	known_bytes buff;
	//unknown bytes after buffer
	skip_type skip = 0;
	//?? ?? ? AA 11 22 BB ?? ?? ? CC 11 0F
	//[  1  ] [         2       ] [  3  ]
	// 1) buff(empty), skip==3
	// 2) buff(size==4) skip==3
	// 3) buff(size==3) skip(empty)
};

class unknown_bytes
{
public:
	using pointer = const unknown_bytes_data*;
	using value_type = unknown_bytes_data;

	void push_back(unknown_bytes_data&& val) noexcept;

	pointer begin() const noexcept;
	pointer end() const noexcept;
	size_t size() const noexcept;

	const unknown_bytes_data& operator[](const size_t index) const noexcept;
	size_t bytes_count() const noexcept;

private:
	std::vector<unknown_bytes_data> data_;
};

export namespace nstd::/*inline*/ mem
{
	using signature_unknown_bytes = unknown_bytes;
	signature_unknown_bytes make_signature(const std::string_view str) runtime_assert_noexcept;
	signature_unknown_bytes make_signature(const char* begin, const size_t mem_size) runtime_assert_noexcept;

	using signature_known_bytes = known_bytes;
	signature_known_bytes make_signature_known(const uint8_t* begin, const size_t mem_size) runtime_assert_noexcept;
}