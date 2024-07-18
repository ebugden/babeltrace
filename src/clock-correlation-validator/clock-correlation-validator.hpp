/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2024 EfficiOS, Inc.
 */

#ifndef BABELTRACE_CLOCK_CORRELATION_VALIDATOR_CLOCK_CORRELATION_VALIDATOR_HPP
#define BABELTRACE_CLOCK_CORRELATION_VALIDATOR_CLOCK_CORRELATION_VALIDATOR_HPP

#include "cpp-common/bt2/message.hpp"

#include "clock-correlation-validator/clock-correlation-validator.h"

namespace bt2ccv {

class ClockCorrelationError final : public std::runtime_error
{
public:
    enum class Type
    {
        ExpectingNoClockClassGotOne =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_NO_CLOCK_CLASS_GOT_ONE,

        ExpectingOriginKnownGotNone =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_KNOWN_GOT_NONE,
        ExpectingOriginKnownGotUnknown =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_KNOWN_GOT_UNKNOWN,
        ExpectingOriginKnownGotWrong =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_KNOWN_GOT_WRONG,

        ExpectingOriginUnknownWithIdGotNone =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITH_ID_GOT_NONE,
        ExpectingOriginUnknownWithIdGotKnown =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITH_ID_GOT_KNOWN,
        ExpectingOriginUnknownWithIdGotWithout =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITH_ID_GOT_WITHOUT,
        ExpectingOriginUnknownWithIdGotWrong =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITH_ID_GOT_WRONG,

        ExpectingOriginUnknownWithoutIdGotNone =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITHOUT_ID_GOT_NONE,
        ExpectingOriginUnknownWithoutIdGotWrong =
            BT_CLOCK_CORRELATION_VALIDATOR_ERROR_TYPE_EXPECTING_ORIGIN_UNKNOWN_WITHOUT_ID_GOT_WRONG,
    };

    explicit ClockCorrelationError(
        Type type, const bt2::OptionalBorrowedObject<bt2::ConstClockClass> actualClockCls,
        const bt2::OptionalBorrowedObject<bt2::ConstClockClass> refClockCls,
        const bt2::OptionalBorrowedObject<bt2::ConstStreamClass> streamCls) noexcept :
        std::runtime_error {"Clock classes are not correlatable"},
        _mType {type}, _mActualClockCls {actualClockCls}, _mRefClockCls {refClockCls},
        _mStreamCls {streamCls}

    {
    }

    Type type() const noexcept
    {
        return _mType;
    }

    bt2::OptionalBorrowedObject<bt2::ConstClockClass> actualClockCls() const noexcept
    {
        return _mActualClockCls;
    }

    bt2::OptionalBorrowedObject<bt2::ConstClockClass> refClockCls() const noexcept
    {
        return _mRefClockCls;
    }

    bt2::OptionalBorrowedObject<bt2::ConstStreamClass> streamCls() const noexcept
    {
        return _mStreamCls;
    }

private:
    Type _mType;
    bt2::OptionalBorrowedObject<bt2::ConstClockClass> _mActualClockCls;
    bt2::OptionalBorrowedObject<bt2::ConstClockClass> _mRefClockCls;
    bt2::OptionalBorrowedObject<bt2::ConstStreamClass> _mStreamCls;
};

class ClockCorrelationValidator final
{
private:
    enum class PropsExpectation
    {
        /* We haven't recorded clock properties yet. */
        Unset,

        /* Expect to have no clock. */
        None,

        /* Expect a clock with a known origin. */
        OriginKnown,

        /* Expect a clock with an unknown origin, but with an identify. */
        OriginUnknownWithId,

        /* Expect a clock with an unknown origin and without an identify. */
        OriginUnknownWithoutId,
    };

public:
    void validate(const bt2::ConstMessage msg, const int graphMipVersion)
    {
        if (!msg.isStreamBeginning() && !msg.isMessageIteratorInactivity()) {
            return;
        }

        this->_validate(msg, graphMipVersion);
    }

private:
    void _validate(const bt2::ConstMessage msg, int graphMipVersion);

    PropsExpectation _mExpectation = PropsExpectation::Unset;

    /*
     * Reference clock class: the clock class used to set expectations.
     *
     * To make sure that the clock class pointed to by this member
     * doesn't get freed and another one reallocated at the same
     * address, keep a strong reference, ensuring that it lives at least
     * as long as the owner of this validator.
     */
    bt2::ConstClockClass::Shared _mRefClockClass;
};

} /* namespace bt2ccv */

#endif /* BABELTRACE_CLOCK_CORRELATION_VALIDATOR_CLOCK_CORRELATION_VALIDATOR_HPP */
