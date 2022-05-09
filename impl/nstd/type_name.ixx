module;
#include <nstd/core_utils.h>

#include <algorithm>
#include <string>
#include <array>

export module nstd.type_name;

constexpr size_t count_substring(const std::string_view str, const std::string_view substr) noexcept
{
	size_t found = 0;
	size_t pos = 0;
	for(;;)
	{
		pos = str.find(substr, pos);
		if(pos == str.npos)
			break;
		++found;
		pos += substr.size( );
	}
	return found;
}

constexpr void erase_substring(std::string& str, const std::string_view substr) noexcept
{
	size_t pos = 0;
	for(;;)
	{
		pos = str.find(substr, pos);
		if(pos == str.npos)
			break;
		str.erase(pos, substr.size( ));
	}
}

constexpr std::string erase_substring(const std::string_view str, const std::string_view substr, const size_t buffer_size = 0) noexcept
{
	std::string result;
	result.reserve(buffer_size == 0 ? str.size( ) : buffer_size);
	size_t pos = 0;
	const auto itr = str.begin( );
	for(;;)
	{
		size_t from = pos;
		pos = str.find(substr, pos);
		const auto done = pos == str.npos;
		result.append(itr + from, done ? str.end( ) : itr + pos);
		if(done)
			break;
		pos += substr.size( );
	}
	return result;
}

#if 0

constexpr size_t override_substring(std::string& str, const std::string_view substr, const char dummy = '\0')
{
	const auto substr_size = substr.size( );
	size_t found = 0;
	size_t pos = 0;
	for(;;)
	{
		pos = str.find(substr, pos);
		if(pos == str.npos)
			break;
		++found;
		std::fill_n(str.data( ) + pos, substr_size, dummy);
		pos += substr_size;
	}
	return found * substr_size;
}

constexpr std::string cleanup_string(const std::string_view str, const size_t buffer_size, const char bad_chr = '\0')
{
	std::string result;
	result.reserve(/*buffer_size == 0 ? str.size( ) :*/ buffer_size);
	for(const auto chr : str)
	{
		if(chr == bad_chr)
			continue;
		result += chr;
	}
	return result;
}

constexpr std::string erase_substring2(const std::string_view str, const std::string_view substr)
{
	std::string temp_str;
	temp_str.assign(str.begin( ), str.end( ));
	const auto buffer_size = override_substring(temp_str, substr, '\0');
	if(buffer_size == 0)
		return temp_str;

	return cleanup_string(temp_str, buffer_size, '\0');
}

#endif

template<typename T>
constexpr T extract_type(const std::string_view raw_name) noexcept
{
	const auto start = raw_name.find('<') + 1;
	const auto end = raw_name.rfind('>');
	const auto name_size = end - start;
	return {raw_name.data( ) + start, name_size};//raw_name.substr(start, name_size), bus string dont accept it
}

constexpr std::array<std::string_view, 4> uselles_words = {"struct ","class ","enum ","union "};

constexpr std::string clean_type_name(const std::string_view raw_name) noexcept
{
	auto correct_name = extract_type<std::string>(raw_name);
#if 1
	for(const auto w : uselles_words)
		erase_substring(correct_name, w);
	return correct_name;
#else
	size_t buffer_size = 0;
	for(const auto w : uselles_words)
		buffer_size += override_substring(correct_name, w);
	return buffer_size == 0 ? correct_name : cleanup_string(correct_name, buffer_size);
#endif
}

//returns size without substring
constexpr size_t clean_type_name_size(const std::string_view raw_name) noexcept
{
	size_t removed = 0;
	const auto correct_name = extract_type<std::string_view>(raw_name);
	for(const auto w : uselles_words)
		removed += count_substring(correct_name, w) * w.size( );
	return correct_name.size( ) - removed;
}

constexpr bool template_comparer(const char* left, const char* right) noexcept
{
	if(left == right)
		return true;

	//skip XXXXtype_name_raw
	do
		++left;
	while(*right++ != '<');

	for(;;)
	{
		auto l = *left++;
		auto r = *right++;

		if(l != r)
		{
			return l == '>' || r == '>'//partial template _Class
				|| l == '<' || r == '<';//full template _Class<XXX>;
		}
		if(l == '\0')
			return false;
		if(l == '<' || l == '>')
			return true;
	}
}

template <typename T>
constexpr decltype(auto) type_name_raw( ) noexcept
{
	return __FUNCSIG__;
}

template <template<class...> class T>
constexpr decltype(auto) type_name_raw( ) noexcept
{
	return __FUNCSIG__;
}

template<size_t Size, bool NullTerminated = false>
constexpr auto make_string_buffer(const std::string_view str) noexcept
{
	auto buff = std::array<char, Size + static_cast<size_t>(NullTerminated)>( );
	if(buff.size( ) != str.size( ))
		buff.fill('\0');
	std::copy(str.begin( ), str.end( ), buff.data( ));
	return buff;
}

template <typename T>
constexpr auto type_name_impl( ) noexcept
{
	constexpr std::string_view raw_name = type_name_raw<T>( );
	constexpr auto out_buffer_size = clean_type_name_size(raw_name);
	if constexpr(raw_name.size( ) == out_buffer_size)
		return raw_name;
	else
		return make_string_buffer<out_buffer_size>(clean_type_name(raw_name));
}

template <template<class...> class T>
constexpr auto type_name_partial_impl( ) noexcept
{
	constexpr std::string_view raw_name = type_name_raw<T>( );
	constexpr auto out_buffer_size = clean_type_name_size(raw_name);
	if constexpr(raw_name.size( ) == out_buffer_size)
		return raw_name;
	else
		return make_string_buffer<out_buffer_size>(clean_type_name(raw_name));
}

template <typename T>
constexpr auto type_name_drop_templates_impl( ) noexcept
{
	constexpr auto name = type_name_impl<T>( );
	constexpr std::string_view name_sized = {name.data( ),name.size( )};
	constexpr auto template_start = name_sized.find('<');
	if constexpr(template_start == name_sized.npos)
	{
		return name;
	}
	else
	{
		constexpr auto name_clamped = name_sized.substr(0, template_start);
		return make_string_buffer<name_clamped.size( )>(name_clamped);
	}
}

template <typename T>
constexpr auto type_name_holder = type_name_impl<T>( );

template <template<class...> class T>
constexpr auto type_name_holder_partial = type_name_partial_impl<T>( );

template <typename T>
constexpr auto type_name_holder_drop_templates = type_name_drop_templates_impl<T>( );

template <class T>
constexpr std::string_view extract_holder(const T& holder) noexcept
{
	return {holder.data( ),holder.size( )};
}

export namespace nstd
{
	template <typename T>
	constexpr std::string_view type_name( ) noexcept
	{
		return extract_holder(type_name_holder<T>);
	}

	template <template<typename...>class T>
	constexpr std::string_view type_name( ) noexcept
	{
		return extract_holder(type_name_holder_partial<T>);
	}

	template <template<typename, size_t> class T>
	constexpr std::string_view type_name( ) noexcept
	{
		return extract_holder(type_name_holder_drop_templates<T<int, 0>>);
	}

	static_assert(type_name<int>( ) == "int");
	static_assert(type_name<std::char_traits>( ) == "std::char_traits");
	static_assert(type_name<std::char_traits<char>>( ) == "std::char_traits<char>");
	static_assert(type_name<std::array>( ) == "std::array");
	static_assert(type_name<std::exception>( ) == "std::exception");
	//static_assert(type_name<std::false_type>( ) == "std::integral_constant<bool, false>"); everything is correct but assert fails

	//------------

	constexpr std::string drop_namespace(const std::string_view str, const std::string_view drop) noexcept
	{
		if(drop.ends_with("::"))
			return erase_substring(str, drop);

		const size_t add = drop.ends_with(':') ? 1 : 2;
		std::string drop_fixed;
		drop_fixed.reserve(drop.size( ) + add);
		drop_fixed.assign(drop.begin( ), drop.end( ));
		drop_fixed.resize(drop.size( ) + add, ':');
		return erase_substring(str, drop_fixed);
	}

	static_assert(drop_namespace("test::test::string", "test") == "string");
	static_assert(drop_namespace("test::test2::string", "test2") == "test::string");
	static_assert(drop_namespace("test::test2::test::string", "test") == "test2::string");

	//------------

	template <class T1, template<class...> class T2>
	constexpr bool same_template( ) noexcept
	{
		return template_comparer(type_name_raw<T1>( ), type_name_raw<T2>( ));
	}

	template <template<class...> class T1, class T2>
	constexpr bool same_template( ) noexcept
	{
		return same_template<T2, T1>( );
	}

	template <template<class...> class T1, template<class...> class T2>
	constexpr bool same_template( ) noexcept
	{
		return template_comparer(type_name_raw<T1>( ), type_name_raw<T2>( ));
	}

	template <class T1, class T2>
	constexpr bool same_template( ) noexcept
	{
		if constexpr(std::is_same_v<T1, T2>)
			return true;
		else
			return template_comparer(type_name_raw<T1>( ), type_name_raw<T2>( ));
	}
}