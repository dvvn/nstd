#pragma once

#if 0
#include "type_traits.h"
#include "runtime_assert_fwd.h"

#include <string>

namespace nstd
{
	template <class _Elem, class _Traits = std::char_traits<_Elem>, class _Base = std::basic_string_view<_Elem, _Traits>>
	class basic_string_view : public _Base
	{
	public:
		using _Base::basic_string_view;

		template <size_t N>
		NSTD_CONSTEXPR_CONTAINTER basic_string_view(const _Elem(&str)[N]) : _Base(str, N - 1)
		{
			runtime_assert(this->size() == _Base(in).size(), "Use begin-end constructor!");
		}
	};

	template <class _Elem, class _Traits = std::char_traits<_Elem>, class _Alloc = std::allocator<_Elem>, class _Base = std::basic_string<_Elem, _Traits, _Alloc>>
	class basic_string : public _Base
	{
	public:
		using _Base::basic_string;

		template <size_t N>
		NSTD_CONSTEXPR_CONTAINTER basic_string(const _Elem(&str)[N]) : _Base(basic_string_view<_Elem, _Traits>(str))
		{
		}

	private:
		template<class _View>
		NSTD_CONSTEXPR_CONTAINTER size_t find_erase(const _View& substr, size_t offset)
			requires(std::constructible_from<_Base, _View>)
		{
			const auto pos = find(substr, offset);
			if (pos != npos)
				erase(pos, substr.size());
			return pos;
		}

	public:
		template<class _View>
		NSTD_CONSTEXPR_CONTAINTER basic_string& erase_all(const _View& substr)
		{
			size_t pos = 0;
			do
			{
				pos = find_erase(substr, pos);
			} while (pos != npos);
			return *this;
		}

		template<class _View>
		NSTD_CONSTEXPR_CONTAINTER basic_string& erase(const _View& substr)
		{
			find_erase(substr, 0);
			return *this;
		}
	};
}
#endif