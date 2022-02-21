module;

#include <string>

module nstd.text.hashed_string;

#define NSTD_HASHED_STRING_PROVIDE(_TYPE_) \
template struct basic_hashed_string<_TYPE_>;\
template struct basic_hashed_string_view<_TYPE_>;

NSTD_HASHED_STRING_PROVIDE(char);
NSTD_HASHED_STRING_PROVIDE(char16_t);
NSTD_HASHED_STRING_PROVIDE(char32_t);
NSTD_HASHED_STRING_PROVIDE(wchar_t);
#ifdef __cpp_lib_char8_t
NSTD_HASHED_STRING_PROVIDE(char8_t);
#endif
