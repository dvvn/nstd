#pragma once

#include <string>

#define CVT_OP_char CVT_OP(char)
#define CVT_OP_wchar CVT_OP(wchar_t)
#define CVT_OP_char16 CVT_OP(char16_t)
#define CVT_OP_char32 CVT_OP(char32_t)

#ifdef __cpp_lib_char8_t
#define CVT_OP_char8 CVT_OP(char8_t)
#define CVT_char8_t \
/*CVT_OP_char8;*/\
CVT_OP_char;\
CVT_OP_wchar;\
CVT_OP_char16;\
CVT_OP_char32;
#else
#define CVT_OP_char8
#define CVT_char8_t
#endif

#define CVT_char \
CVT_OP_char8;\
/*CVT_OP_char;*/\
CVT_OP_wchar;\
CVT_OP_char16;\
CVT_OP_char32;\
CVT_OP_SELF

#define CVT_wchar_t \
CVT_OP_char8;\
CVT_OP_char;\
/*CVT_OP_wchar;*/\
CVT_OP_char16;\
CVT_OP_char32;\
CVT_OP_SELF

#define CVT_char16_t \
CVT_OP_char8;\
CVT_OP_char;\
CVT_OP_wchar;\
/*CVT_OP_char16;*/\
CVT_OP_char32;\
CVT_OP_SELF

#define CVT_char32_t \
CVT_OP_char8;\
CVT_OP_char;\
CVT_OP_wchar;\
CVT_OP_char16;\
/*CVT_OP_char32;*/\
CVT_OP_SELF