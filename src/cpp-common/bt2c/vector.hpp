/*
 * SPDX-FileCopyrightText: 2022 Simon Marchi <simon.marchi@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_VECTOR_HPP
#define BABELTRACE_CPP_COMMON_BT2C_VECTOR_HPP

#include <vector>

#include "common/assert.h"

namespace bt2c {

/*
 * Moves the last entry of `vec` to the index `idx`, then removes the last entry.
 *
 * Meant to be a direct replacement for g_ptr_array_remove_index_fast(), but for
 * `std::vector`.
 */
template <typename T, typename AllocatorT>
void vectorFastRemove(std::vector<T, AllocatorT>& vec,
                      const typename std::vector<T, AllocatorT>::size_type idx)
{
    BT_ASSERT_DBG(idx < vec.size());

    if (idx < vec.size() - 1) {
        vec[idx] = std::move(vec.back());
    }

    vec.pop_back();
}

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_VECTOR_HPP */
