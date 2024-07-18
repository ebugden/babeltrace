/*
 * Copyright (c) 2023 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_BORROWED_OBJECT_PROXY_HPP
#define BABELTRACE_CPP_COMMON_BT2_BORROWED_OBJECT_PROXY_HPP

namespace bt2 {

/*
 * A proxy containing a valid borrowed object instance of `ObjT` to make
 * Something::operator->() work when only a libbabeltrace2 object
 * pointer is available.
 */
template <typename ObjT>
class BorrowedObjectProxy final
{
public:
    explicit BorrowedObjectProxy(typename ObjT::LibObjPtr libObjPtr) noexcept : _mObj {libObjPtr}
    {
    }

    ObjT *operator->() noexcept
    {
        return &_mObj;
    }

    const ObjT *operator->() const noexcept
    {
        return &_mObj;
    }

private:
    ObjT _mObj;
};

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_BORROWED_OBJECT_PROXY_HPP */
