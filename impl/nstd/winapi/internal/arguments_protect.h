#pragma once

#include <nstd/runtime_assert.h>

import nstd.text.chars_cache;

namespace nstd::winapi
{
    template <text::chars_cache... Args>
    void _Protect_args(const char* func_sig)
    {
        static auto func_sig_stored = func_sig;
        runtime_assert(func_sig_stored == func_sig, "Already called with different template args");
    }
} // namespace nstd::winapi
