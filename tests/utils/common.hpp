/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2024 EfficiOS Inc.
 */

#ifndef BABELTRACE_TESTS_UTILS_COMMON_HPP
#define BABELTRACE_TESTS_UTILS_COMMON_HPP

#include <cstdint>

template <typename FuncT>
void forEachMipVersion(const FuncT func)
{
    constexpr std::uint64_t maxGraphMipVersion = 1;

    for (uint64_t v = 0; v <= maxGraphMipVersion; ++v) {
        func(v);
    }
}

#endif /* BABELTRACE_TESTS_UTILS_COMMON_HPP */
