module;

#include <string>
#include <variant>

export module nstd.text.string_or_view;

template<typename T>
class basic_string_or_view_holder
{
public:

	using value_type = T;
	using string_type = std::basic_string<T>;
	using view_type = std::basic_string_view<T>;

	basic_string_or_view_holder( )
	{
		str_.emplace<view_type>( );
	}
	basic_string_or_view_holder(const view_type sv)
	{
		str_.emplace<view_type>(sv);
	}
	basic_string_or_view_holder(string_type&& str)
	{
		str_.emplace<string_type>(std::move(str));
	}
	basic_string_or_view_holder(const T* str)
	{
		str_.emplace<view_type>(str);
	}

	basic_string_or_view_holder(string_type& str) = delete;

	operator string_type& ()&
	{
		return std::get<string_type>(str_);
	}
	operator string_type( )&&
	{
		constexpr auto small_string_size = string_type( ).capacity( ) - 1;
		return std::visit([]<class S>(S & ref)-> string_type
		{
			if constexpr (std::is_same_v<S, string_type>)
			{
				if (ref.size( ) > small_string_size)
					return std::move(ref);
			}

			return {ref.data( ),ref.size( )};
		}, str_);
	}
	operator view_type( ) const
	{
		return std::visit([]<class S>(const S & ref)-> view_type
		{
			return {ref.data( ), ref.size( )};
		}, str_);
	}

private:
	std::variant<string_type, view_type> str_;
};

export namespace nstd::inline text
{
	using string_or_view_holder = basic_string_or_view_holder<char>;
}