/*
 * Copyright (c) 2020 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP
#define BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP

#include <cstdint>
#include <type_traits>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/uuid.hpp"
#include "cpp-common/bt2s/optional.hpp"

#include "borrowed-object.hpp"
#include "exc.hpp"
#include "internal/utils.hpp"
#include "shared-object.hpp"
#include "value.hpp"

namespace bt2 {
namespace internal {

struct ClockClassRefFuncs final
{
    static void get(const bt_clock_class * const libObjPtr) noexcept
    {
        bt_clock_class_get_ref(libObjPtr);
    }

    static void put(const bt_clock_class * const libObjPtr) noexcept
    {
        bt_clock_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonClockClassSpec;

/* Functions specific to mutable clock classes */
template <>
struct CommonClockClassSpec<bt_clock_class> final
{
    static bt_value *userAttributes(bt_clock_class * const libObjPtr) noexcept
    {
        return bt_clock_class_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant clock classes */
template <>
struct CommonClockClassSpec<const bt_clock_class> final
{
    static const bt_value *userAttributes(const bt_clock_class * const libObjPtr) noexcept
    {
        return bt_clock_class_borrow_user_attributes_const(libObjPtr);
    }
};

} /* namespace internal */

class ClockOffset final
{
public:
    explicit ClockOffset(const std::int64_t seconds, const std::uint64_t cycles) :
        _mSeconds {seconds}, _mCycles {cycles}
    {
    }

    std::int64_t seconds() const noexcept
    {
        return _mSeconds;
    }

    std::uint64_t cycles() const noexcept
    {
        return _mCycles;
    }

private:
    std::int64_t _mSeconds;
    std::uint64_t _mCycles;
};

template <typename LibObjT>
class CommonClockClass final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonClockClass, LibObjT, internal::ClockClassRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    explicit CommonClockClass(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonClockClass(const CommonClockClass<OtherLibObjT> clkClass) noexcept :
        _ThisBorrowedObject {clkClass}
    {
    }

    template <typename OtherLibObjT>
    CommonClockClass& operator=(const CommonClockClass<OtherLibObjT> clkClass) noexcept
    {
        _ThisBorrowedObject::operator=(clkClass);
        return *this;
    }

    CommonClockClass<const bt_clock_class> asConst() const noexcept
    {
        return CommonClockClass<const bt_clock_class> {*this};
    }

    CommonClockClass frequency(const std::uint64_t frequency) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        bt_clock_class_set_frequency(this->libObjPtr(), frequency);
        return *this;
    }

    std::uint64_t frequency() const noexcept
    {
        return bt_clock_class_get_frequency(this->libObjPtr());
    }

    CommonClockClass offsetFromOrigin(const ClockOffset& offsetFromOrigin) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        bt_clock_class_set_offset(this->libObjPtr(), offsetFromOrigin.seconds(),
                                  offsetFromOrigin.cycles());
        return *this;
    }

    ClockOffset offsetFromOrigin() const noexcept
    {
        std::int64_t seconds;
        std::uint64_t cycles;

        bt_clock_class_get_offset(this->libObjPtr(), &seconds, &cycles);
        return ClockOffset {seconds, cycles};
    }

    CommonClockClass precision(const std::uint64_t precision) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        bt_clock_class_set_precision(this->libObjPtr(), precision);
        return *this;
    }

    std::uint64_t precision() const noexcept
    {
        return bt_clock_class_get_precision(this->libObjPtr());
    }

    CommonClockClass originIsUnixEpoch(const bool originIsUnixEpoch) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        bt_clock_class_set_origin_is_unix_epoch(this->libObjPtr(),
                                                static_cast<bt_bool>(originIsUnixEpoch));
        return *this;
    }

    bool originIsUnixEpoch() const noexcept
    {
        return static_cast<bool>(bt_clock_class_origin_is_unix_epoch(this->libObjPtr()));
    }

    CommonClockClass name(const bt2c::CStringView name) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        const auto status = bt_clock_class_set_name(this->libObjPtr(), name);

        if (status == BT_CLOCK_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_clock_class_get_name(this->libObjPtr());
    }

    CommonClockClass description(const bt2c::CStringView description) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        const auto status = bt_clock_class_set_description(this->libObjPtr(), description);

        if (status == BT_CLOCK_CLASS_SET_DESCRIPTION_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView description() const noexcept
    {
        return bt_clock_class_get_description(this->libObjPtr());
    }

    CommonClockClass uuid(const bt2c::UuidView uuid) const noexcept
    {
        bt_clock_class_set_uuid(this->libObjPtr(), uuid.data());
        return *this;
    }

    bt2s::optional<bt2c::UuidView> uuid() const noexcept
    {
        const auto uuid = bt_clock_class_get_uuid(this->libObjPtr());

        if (uuid) {
            return bt2c::UuidView {uuid};
        }

        return bt2s::nullopt;
    }

    template <typename LibValT>
    CommonClockClass userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstClockClass`.");

        bt_clock_class_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {
            internal::CommonClockClassSpec<LibObjT>::userAttributes(this->libObjPtr())};
    }

    std::int64_t cyclesToNsFromOrigin(const std::uint64_t value) const
    {
        std::int64_t nsFromOrigin;
        const auto status =
            bt_clock_class_cycles_to_ns_from_origin(this->libObjPtr(), value, &nsFromOrigin);

        if (status == BT_CLOCK_CLASS_CYCLES_TO_NS_FROM_ORIGIN_STATUS_OVERFLOW_ERROR) {
            throw OverflowError {};
        }

        return nsFromOrigin;
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using ClockClass = CommonClockClass<bt_clock_class>;
using ConstClockClass = CommonClockClass<const bt_clock_class>;

namespace internal {

struct ClockClassTypeDescr
{
    using Const = ConstClockClass;
    using NonConst = ClockClass;
};

template <>
struct TypeDescr<ClockClass> : public ClockClassTypeDescr
{
};

template <>
struct TypeDescr<ConstClockClass> : public ClockClassTypeDescr
{
};

} /* namespace internal */
} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_CLOCK_CLASS_HPP */
