/*
 * Copyright (c) 2024 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_IDENTITY_HPP
#define BABELTRACE_CPP_COMMON_BT2_IDENTITY_HPP

#include "cpp-common/bt2c/c-string-view.hpp"

namespace bt2 {

class IdentityView final
{
public:
    explicit IdentityView(const bt2c::CStringView nameSpace, const bt2c::CStringView name,
                          const bt2c::CStringView uid) :
        _mNamespace {nameSpace},
        _mName {name}, _mUid {uid}
    {
    }

    bt2c::CStringView nameSpace() const noexcept
    {
        return _mNamespace;
    }

    bt2c::CStringView name() const noexcept
    {
        return _mName;
    }

    bt2c::CStringView uid() const noexcept
    {
        return _mUid;
    }

private:
    bt2c::CStringView _mNamespace;
    bt2c::CStringView _mName;
    bt2c::CStringView _mUid;
};

inline bool same(const IdentityView& a, const IdentityView& b) noexcept
{
    BT_ASSERT_DBG(a.name());
    BT_ASSERT_DBG(a.uid());
    BT_ASSERT_DBG(b.name());
    BT_ASSERT_DBG(b.uid());

    return equalsMaybeNullptr(a.nameSpace(), b.nameSpace()) && a.name() == b.name() &&
           a.uid() == b.uid();
}

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_IDENTITY_HPP */
