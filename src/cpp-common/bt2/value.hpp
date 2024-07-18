/*
 * Copyright (c) 2020 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_VALUE_HPP
#define BABELTRACE_CPP_COMMON_BT2_VALUE_HPP

#include <cstdint>
#include <functional>
#include <type_traits>

#include <babeltrace2/babeltrace.h>

#include "common/assert.h"
#include "common/common.h"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/vendor/wise-enum/wise_enum.h"

#include "borrowed-object-iterator.hpp"
#include "borrowed-object.hpp"
#include "exc.hpp"
#include "internal/utils.hpp"
#include "optional-borrowed-object.hpp"
#include "raw-value-proxy.hpp"
#include "shared-object.hpp"

namespace bt2 {
namespace internal {

struct ValueRefFuncs final
{
    static void get(const bt_value * const libObjPtr) noexcept
    {
        bt_value_get_ref(libObjPtr);
    }

    static void put(const bt_value * const libObjPtr) noexcept
    {
        bt_value_put_ref(libObjPtr);
    }
};

} /* namespace internal */

template <typename ObjT, typename LibObjT>
using SharedValue = SharedObject<ObjT, LibObjT, internal::ValueRefFuncs>;

template <typename LibObjT>
class CommonNullValue;

template <typename LibObjT>
class CommonBoolValue;

template <typename LibObjT>
class CommonUnsignedIntegerValue;

template <typename LibObjT>
class CommonSignedIntegerValue;

template <typename LibObjT>
class CommonRealValue;

template <typename LibObjT>
class CommonStringValue;

template <typename LibObjT>
class CommonArrayValue;

template <typename LibObjT>
class CommonMapValue;

/* clang-format off */

WISE_ENUM_CLASS(ValueType,
    (Null, BT_VALUE_TYPE_NULL),
    (Bool, BT_VALUE_TYPE_BOOL),
    (UnsignedInteger, BT_VALUE_TYPE_UNSIGNED_INTEGER),
    (SignedInteger, BT_VALUE_TYPE_SIGNED_INTEGER),
    (Real, BT_VALUE_TYPE_REAL),
    (String, BT_VALUE_TYPE_STRING),
    (Array, BT_VALUE_TYPE_ARRAY),
    (Map, BT_VALUE_TYPE_MAP));

/* clang-format on */

template <typename ValueObjT>
class CommonValueRawValueProxy final
{
public:
    explicit CommonValueRawValueProxy(const ValueObjT obj) : _mObj {obj}
    {
    }

    CommonValueRawValueProxy& operator=(bool rawVal) noexcept;
    CommonValueRawValueProxy& operator=(std::int64_t rawVal) noexcept;
    CommonValueRawValueProxy& operator=(std::uint64_t rawVal) noexcept;
    CommonValueRawValueProxy& operator=(double rawVal) noexcept;
    CommonValueRawValueProxy& operator=(const char *rawVal);
    CommonValueRawValueProxy& operator=(bt2c::CStringView rawVal);
    operator bool() const noexcept;
    operator std::int64_t() const noexcept;
    operator std::uint64_t() const noexcept;
    operator double() const noexcept;
    operator bt2c::CStringView() const noexcept;

private:
    ValueObjT _mObj;
};

template <typename LibObjT>
class CommonValue : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;

protected:
    using _ThisCommonValue = CommonValue<LibObjT>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonValue<LibObjT>, LibObjT>;

    explicit CommonValue(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonValue(const CommonValue<OtherLibObjT> val) noexcept : _ThisBorrowedObject {val}
    {
    }

    template <typename OtherLibObjT>
    _ThisCommonValue operator=(const CommonValue<OtherLibObjT> val) noexcept
    {
        _ThisBorrowedObject::operator=(val);
        return *this;
    }

    CommonValue<const bt_value> asConst() const noexcept
    {
        return CommonValue<const bt_value> {*this};
    }

    ValueType type() const noexcept
    {
        return static_cast<ValueType>(bt_value_get_type(this->libObjPtr()));
    }

    bool isNull() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_NULL);
    }

    bool isBool() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_BOOL);
    }

    bool isInteger() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_INTEGER);
    }

    bool isUnsignedInteger() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_UNSIGNED_INTEGER);
    }

    bool isSignedInteger() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_SIGNED_INTEGER);
    }

    bool isReal() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_REAL);
    }

    bool isString() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_STRING);
    }

    bool isArray() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_ARRAY);
    }

    bool isMap() const noexcept
    {
        return this->_libTypeIs(BT_VALUE_TYPE_MAP);
    }

    template <typename OtherLibObjT>
    bool operator==(const CommonValue<OtherLibObjT> other) const noexcept
    {
        return static_cast<bool>(bt_value_is_equal(this->libObjPtr(), other.libObjPtr()));
    }

    template <typename OtherLibObjT>
    bool operator!=(const CommonValue<OtherLibObjT> other) const noexcept
    {
        return !(*this == other);
    }

    CommonValueRawValueProxy<CommonValue> operator*() const noexcept
    {
        return CommonValueRawValueProxy<CommonValue> {*this};
    }

    SharedValue<CommonValue<bt_value>, bt_value> copy() const
    {
        bt_value *copy;

        if (bt_value_copy(this->libObjPtr(), &copy) == BT_VALUE_COPY_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return SharedValue<CommonValue<bt_value>, bt_value>::createWithoutRef(copy);
    }

    std::uint64_t arrayLength() const noexcept
    {
        return this->asArray().length();
    }

    bool arrayIsEmpty() const noexcept
    {
        return this->asArray().isEmpty();
    }

    CommonValue<LibObjT> operator[](const std::uint64_t index) const noexcept
    {
        return this->asArray()[index];
    }

    template <typename T>
    void append(T&& elem) const
    {
        this->asArray().append(std::forward<T>(elem));
    }

    CommonArrayValue<bt_value> appendEmptyArray() const;
    CommonMapValue<bt_value> appendEmptyMap() const;

    std::uint64_t mapLength() const noexcept
    {
        return this->asMap().length();
    }

    bool mapIsEmpty() const noexcept
    {
        return this->asMap().isEmpty();
    }

    template <typename KeyT>
    OptionalBorrowedObject<CommonValue<LibObjT>> operator[](KeyT&& key) const noexcept
    {
        return this->asMap()[std::forward<KeyT>(key)];
    }

    template <typename KeyT>
    bool hasEntry(KeyT&& key) const noexcept
    {
        return this->asMap().hasEntry(std::forward<KeyT>(key));
    }

    template <typename KeyT, typename ValT>
    void insert(KeyT&& key, ValT&& val) const
    {
        this->asMap().insert(std::forward<KeyT>(key), std::forward<ValT>(val));
    }

    CommonArrayValue<bt_value> insertEmptyArray(bt2c::CStringView key) const;
    CommonMapValue<bt_value> insertEmptyMap(bt2c::CStringView key) const;

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }

    template <typename ValueT>
    ValueT as() const noexcept
    {
        return ValueT {this->libObjPtr()};
    }

    CommonNullValue<LibObjT> asNull() const noexcept;
    CommonBoolValue<LibObjT> asBool() const noexcept;
    CommonSignedIntegerValue<LibObjT> asSignedInteger() const noexcept;
    CommonUnsignedIntegerValue<LibObjT> asUnsignedInteger() const noexcept;
    CommonRealValue<LibObjT> asReal() const noexcept;
    CommonStringValue<LibObjT> asString() const noexcept;
    CommonArrayValue<LibObjT> asArray() const noexcept;
    CommonMapValue<LibObjT> asMap() const noexcept;

protected:
    bool _libTypeIs(const bt_value_type type) const noexcept
    {
        return bt_value_type_is(bt_value_get_type(this->libObjPtr()), type);
    }
};

using Value = CommonValue<bt_value>;
using ConstValue = CommonValue<const bt_value>;

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const bool rawVal) noexcept
{
    _mObj.asBool().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const std::int64_t rawVal) noexcept
{
    _mObj.asSignedInteger().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const std::uint64_t rawVal) noexcept
{
    _mObj.asUnsignedInteger().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const double rawVal) noexcept
{
    _mObj.asReal().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const char * const rawVal)
{
    _mObj.asString().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>&
CommonValueRawValueProxy<ValueObjT>::operator=(const bt2c::CStringView rawVal)
{
    _mObj.asString().value(rawVal);
    return *this;
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>::operator bool() const noexcept
{
    return _mObj.asBool().value();
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>::operator std::int64_t() const noexcept
{
    return _mObj.asSignedInteger().value();
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>::operator std::uint64_t() const noexcept
{
    return _mObj.asUnsignedInteger().value();
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>::operator double() const noexcept
{
    return _mObj.asReal().value();
}

template <typename ValueObjT>
CommonValueRawValueProxy<ValueObjT>::operator bt2c::CStringView() const noexcept
{
    return _mObj.asString().value();
}

namespace internal {

struct ValueTypeDescr
{
    using Const = ConstValue;
    using NonConst = Value;
};

template <>
struct TypeDescr<Value> : public ValueTypeDescr
{
};

template <>
struct TypeDescr<ConstValue> : public ValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonNullValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using Shared = SharedValue<CommonNullValue<LibObjT>, LibObjT>;

    CommonNullValue() noexcept : _ThisCommonValue {bt_value_null}
    {
    }

    template <typename OtherLibObjT>
    CommonNullValue(const CommonNullValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonNullValue<LibObjT> operator=(const CommonNullValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonNullValue<const bt_value> asConst() const noexcept
    {
        return CommonNullValue<const bt_value> {*this};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using NullValue = CommonNullValue<bt_value>;
using ConstNullValue = CommonNullValue<const bt_value>;

namespace internal {

struct NullValueTypeDescr
{
    using Const = ConstNullValue;
    using NonConst = NullValue;
};

template <>
struct TypeDescr<NullValue> : public NullValueTypeDescr
{
};

template <>
struct TypeDescr<ConstNullValue> : public NullValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonBoolValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonBoolValue<LibObjT>, LibObjT>;
    using Value = bool;

    explicit CommonBoolValue(const LibObjPtr libObjPtr) noexcept : _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isBool());
    }

    template <typename OtherLibObjT>
    CommonBoolValue(const CommonBoolValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    static Shared create(const Value rawVal = false)
    {
        const auto libObjPtr = bt_value_bool_create_init(static_cast<bt_bool>(rawVal));

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonBoolValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonBoolValue<LibObjT> operator=(const CommonBoolValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonBoolValue<const bt_value> asConst() const noexcept
    {
        return CommonBoolValue<const bt_value> {*this};
    }

    RawValueProxy<CommonBoolValue> operator*() const noexcept
    {
        return RawValueProxy<CommonBoolValue> {*this};
    }

    Value value() const noexcept
    {
        return static_cast<Value>(bt_value_bool_get(this->libObjPtr()));
    }

    CommonBoolValue value(const Value val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstBoolValue`.");

        bt_value_bool_set(this->libObjPtr(), static_cast<bt_bool>(val));
        return *this;
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using BoolValue = CommonBoolValue<bt_value>;
using ConstBoolValue = CommonBoolValue<const bt_value>;

namespace internal {

struct BoolValueTypeDescr
{
    using Const = ConstBoolValue;
    using NonConst = BoolValue;
};

template <>
struct TypeDescr<BoolValue> : public BoolValueTypeDescr
{
};

template <>
struct TypeDescr<ConstBoolValue> : public BoolValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonUnsignedIntegerValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonUnsignedIntegerValue<LibObjT>, LibObjT>;
    using Value = std::uint64_t;

    explicit CommonUnsignedIntegerValue(const LibObjPtr libObjPtr) noexcept :
        _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isUnsignedInteger());
    }

    static Shared create(const Value rawVal = 0)
    {
        const auto libObjPtr = bt_value_integer_unsigned_create_init(rawVal);

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonUnsignedIntegerValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonUnsignedIntegerValue(const CommonUnsignedIntegerValue<OtherLibObjT> val) noexcept :
        _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonUnsignedIntegerValue<LibObjT>
    operator=(const CommonUnsignedIntegerValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonUnsignedIntegerValue<const bt_value> asConst() const noexcept
    {
        return CommonUnsignedIntegerValue<const bt_value> {*this};
    }

    RawValueProxy<CommonUnsignedIntegerValue> operator*() const noexcept
    {
        return RawValueProxy<CommonUnsignedIntegerValue> {*this};
    }

    CommonUnsignedIntegerValue value(const Value val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstUnsignedIntegerValue`.");

        bt_value_integer_unsigned_set(this->libObjPtr(), val);
        return *this;
    }

    Value value() const noexcept
    {
        return bt_value_integer_unsigned_get(this->libObjPtr());
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using UnsignedIntegerValue = CommonUnsignedIntegerValue<bt_value>;
using ConstUnsignedIntegerValue = CommonUnsignedIntegerValue<const bt_value>;

namespace internal {

struct UnsignedIntegerValueTypeDescr
{
    using Const = ConstUnsignedIntegerValue;
    using NonConst = UnsignedIntegerValue;
};

template <>
struct TypeDescr<UnsignedIntegerValue> : public UnsignedIntegerValueTypeDescr
{
};

template <>
struct TypeDescr<ConstUnsignedIntegerValue> : public UnsignedIntegerValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonSignedIntegerValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonSignedIntegerValue<LibObjT>, LibObjT>;
    using Value = std::int64_t;

    explicit CommonSignedIntegerValue(const LibObjPtr libObjPtr) noexcept :
        _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isSignedInteger());
    }

    static Shared create(const Value rawVal = 0)
    {
        const auto libObjPtr = bt_value_integer_signed_create_init(rawVal);

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonSignedIntegerValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonSignedIntegerValue(const CommonSignedIntegerValue<OtherLibObjT> val) noexcept :
        _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonSignedIntegerValue<LibObjT>
    operator=(const CommonSignedIntegerValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonSignedIntegerValue<const bt_value> asConst() const noexcept
    {
        return CommonSignedIntegerValue<const bt_value> {*this};
    }

    RawValueProxy<CommonSignedIntegerValue> operator*() const noexcept
    {
        return RawValueProxy<CommonSignedIntegerValue> {*this};
    }

    CommonSignedIntegerValue value(const Value val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstSignedIntegerValue`.");

        bt_value_integer_signed_set(this->libObjPtr(), val);
        return *this;
    }

    Value value() const noexcept
    {
        return bt_value_integer_signed_get(this->libObjPtr());
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using SignedIntegerValue = CommonSignedIntegerValue<bt_value>;
using ConstSignedIntegerValue = CommonSignedIntegerValue<const bt_value>;

namespace internal {

struct SignedIntegerValueTypeDescr
{
    using Const = ConstSignedIntegerValue;
    using NonConst = SignedIntegerValue;
};

template <>
struct TypeDescr<SignedIntegerValue> : public SignedIntegerValueTypeDescr
{
};

template <>
struct TypeDescr<ConstSignedIntegerValue> : public SignedIntegerValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonRealValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonRealValue<LibObjT>, LibObjT>;
    using Value = double;

    explicit CommonRealValue(const LibObjPtr libObjPtr) noexcept : _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isReal());
    }

    static Shared create(const Value rawVal = 0)
    {
        const auto libObjPtr = bt_value_real_create_init(rawVal);

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonRealValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonRealValue(const CommonRealValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonRealValue<LibObjT> operator=(const CommonRealValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonRealValue<const bt_value> asConst() const noexcept
    {
        return CommonRealValue<const bt_value> {*this};
    }

    RawValueProxy<CommonRealValue> operator*() const noexcept
    {
        return RawValueProxy<CommonRealValue> {*this};
    }

    CommonRealValue value(const Value val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstRealValue`.");

        bt_value_real_set(this->libObjPtr(), val);
        return *this;
    }

    Value value() const noexcept
    {
        return bt_value_real_get(this->libObjPtr());
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using RealValue = CommonRealValue<bt_value>;
using ConstRealValue = CommonRealValue<const bt_value>;

namespace internal {

struct RealValueTypeDescr
{
    using Const = ConstRealValue;
    using NonConst = RealValue;
};

template <>
struct TypeDescr<RealValue> : public RealValueTypeDescr
{
};

template <>
struct TypeDescr<ConstRealValue> : public RealValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
class CommonStringValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonStringValue<LibObjT>, LibObjT>;
    using Value = bt2c::CStringView;

    explicit CommonStringValue(const LibObjPtr libObjPtr) noexcept : _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isString());
    }

    static Shared create(const bt2c::CStringView rawVal = "")
    {
        const auto libObjPtr = bt_value_string_create_init(rawVal);

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonStringValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonStringValue(const CommonStringValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonStringValue<LibObjT> operator=(const CommonStringValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonStringValue<const bt_value> asConst() const noexcept
    {
        return CommonStringValue<const bt_value> {*this};
    }

    RawValueProxy<CommonStringValue> operator*() const noexcept
    {
        return RawValueProxy<CommonStringValue> {*this};
    }

    CommonStringValue value(const Value val) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStringValue`.");

        const auto status = bt_value_string_set(this->libObjPtr(), *val);

        if (status == BT_VALUE_STRING_SET_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    Value value() const noexcept
    {
        return bt_value_string_get(this->libObjPtr());
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using StringValue = CommonStringValue<bt_value>;
using ConstStringValue = CommonStringValue<const bt_value>;

namespace internal {

struct StringValueTypeDescr
{
    using Const = ConstStringValue;
    using NonConst = StringValue;
};

template <>
struct TypeDescr<StringValue> : public StringValueTypeDescr
{
};

template <>
struct TypeDescr<ConstStringValue> : public StringValueTypeDescr
{
};

template <typename LibObjT>
struct CommonArrayValueSpec;

/* Functions specific to mutable array values */
template <>
struct CommonArrayValueSpec<bt_value> final
{
    static bt_value *elementByIndex(bt_value * const libValPtr, const std::uint64_t index) noexcept
    {
        return bt_value_array_borrow_element_by_index(libValPtr, index);
    }
};

/* Functions specific to constant array values */
template <>
struct CommonArrayValueSpec<const bt_value> final
{
    static const bt_value *elementByIndex(const bt_value * const libValPtr,
                                          const std::uint64_t index) noexcept
    {
        return bt_value_array_borrow_element_by_index_const(libValPtr, index);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonArrayValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonArrayValue<LibObjT>, LibObjT>;
    using Iterator = BorrowedObjectIterator<CommonArrayValue<LibObjT>>;

    explicit CommonArrayValue(const LibObjPtr libObjPtr) noexcept : _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isArray());
    }

    static Shared create()
    {
        const auto libObjPtr = bt_value_array_create();

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonArrayValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonArrayValue(const CommonArrayValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonArrayValue<LibObjT> operator=(const CommonArrayValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonArrayValue<const bt_value> asConst() const noexcept
    {
        return CommonArrayValue<const bt_value> {*this};
    }

    std::uint64_t length() const noexcept
    {
        return bt_value_array_get_length(this->libObjPtr());
    }

    Iterator begin() const noexcept
    {
        return Iterator {*this, 0};
    }

    Iterator end() const noexcept
    {
        return Iterator {*this, this->length()};
    }

    bool isEmpty() const noexcept
    {
        return this->length() == 0;
    }

    CommonValue<LibObjT> operator[](const std::uint64_t index) const noexcept
    {
        return CommonValue<LibObjT> {
            internal::CommonArrayValueSpec<LibObjT>::elementByIndex(this->libObjPtr(), index)};
    }

    CommonArrayValue append(const Value val) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status = bt_value_array_append_element(this->libObjPtr(), val.libObjPtr());

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const bool rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status =
            bt_value_array_append_bool_element(this->libObjPtr(), static_cast<bt_bool>(rawVal));

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const std::uint64_t rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status =
            bt_value_array_append_unsigned_integer_element(this->libObjPtr(), rawVal);

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const std::int64_t rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status = bt_value_array_append_signed_integer_element(this->libObjPtr(), rawVal);

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const double rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status = bt_value_array_append_real_element(this->libObjPtr(), rawVal);

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const char * const rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

        const auto status = bt_value_array_append_string_element(this->libObjPtr(), rawVal);

        this->_handleAppendLibStatus(status);
        return *this;
    }

    CommonArrayValue append(const bt2c::CStringView rawVal) const
    {
        return this->append(rawVal.data());
    }

    CommonArrayValue<bt_value> appendEmptyArray() const;
    CommonMapValue<bt_value> appendEmptyMap() const;

    void operator+=(const Value val) const
    {
        this->append(val);
    }

    void operator+=(const bool rawVal) const
    {
        this->append(rawVal);
    }

    void operator+=(const std::uint64_t rawVal) const
    {
        this->append(rawVal);
    }

    void operator+=(const std::int64_t rawVal) const
    {
        this->append(rawVal);
    }

    void operator+=(const double rawVal) const
    {
        this->append(rawVal);
    }

    void operator+=(const char * const rawVal) const
    {
        this->append(rawVal);
    }

    void operator+=(const bt2c::CStringView rawVal) const
    {
        this->append(rawVal);
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }

private:
    void _handleAppendLibStatus(const bt_value_array_append_element_status status) const
    {
        if (status == BT_VALUE_ARRAY_APPEND_ELEMENT_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }
    }
};

using ArrayValue = CommonArrayValue<bt_value>;
using ConstArrayValue = CommonArrayValue<const bt_value>;

namespace internal {

struct ArrayValueTypeDescr
{
    using Const = ConstArrayValue;
    using NonConst = ArrayValue;
};

template <>
struct TypeDescr<ArrayValue> : public ArrayValueTypeDescr
{
};

template <>
struct TypeDescr<ConstArrayValue> : public ArrayValueTypeDescr
{
};

/*
 * Type of a user function passed to `CommonMapValue<ObjT>::forEach()`.
 *
 * First argument is the entry's key, second is its value.
 */
template <typename ObjT>
using CommonMapValueForEachUserFunc = std::function<void(bt2c::CStringView, ObjT)>;

/*
 * Template of a function to be passed to bt_value_map_foreach_entry()
 * for bt_value_map_foreach_entry_const() which calls a user function.
 *
 * `userData` is casted to a `const` pointer to
 * `CommonMapValueForEachUserFunc<ObjT>` (the user function to call).
 *
 * This function catches any exception which the user function throws
 * and returns the `ErrorStatus` value. If there's no exception, this
 * function returns the `OkStatus` value.
 */
template <typename ObjT, typename LibObjT, typename LibStatusT, int OkStatus, int ErrorStatus>
LibStatusT mapValueForEachLibFunc(const char * const key, LibObjT * const libObjPtr,
                                  void * const userData)
{
    const auto& userFunc = *reinterpret_cast<const CommonMapValueForEachUserFunc<ObjT> *>(userData);

    try {
        userFunc(key, ObjT {libObjPtr});
    } catch (...) {
        return static_cast<LibStatusT>(ErrorStatus);
    }

    return static_cast<LibStatusT>(OkStatus);
}

template <typename LibObjT>
struct CommonMapValueSpec;

/* Functions specific to mutable map values */
template <>
struct CommonMapValueSpec<bt_value> final
{
    static bt_value *entryByKey(bt_value * const libValPtr, const char * const key) noexcept
    {
        return bt_value_map_borrow_entry_value(libValPtr, key);
    }

    static void forEach(bt_value * const libValPtr,
                        const CommonMapValueForEachUserFunc<Value>& func)
    {
        const auto status = bt_value_map_foreach_entry(
            libValPtr,
            mapValueForEachLibFunc<Value, bt_value, bt_value_map_foreach_entry_func_status,
                                   BT_VALUE_MAP_FOREACH_ENTRY_FUNC_STATUS_OK,
                                   BT_VALUE_MAP_FOREACH_ENTRY_FUNC_STATUS_ERROR>,
            const_cast<void *>(reinterpret_cast<const void *>(&func)));

        switch (status) {
        case BT_VALUE_MAP_FOREACH_ENTRY_STATUS_OK:
            return;
        case BT_VALUE_MAP_FOREACH_ENTRY_STATUS_USER_ERROR:
        case BT_VALUE_MAP_FOREACH_ENTRY_STATUS_ERROR:
            throw Error {};
        default:
            bt_common_abort();
        }
    }
};

/* Functions specific to constant map values */
template <>
struct CommonMapValueSpec<const bt_value> final
{
    static const bt_value *entryByKey(const bt_value * const libValPtr,
                                      const char * const key) noexcept
    {
        return bt_value_map_borrow_entry_value_const(libValPtr, key);
    }

    static void forEach(const bt_value * const libValPtr,
                        const CommonMapValueForEachUserFunc<ConstValue>& func)
    {
        const auto status = bt_value_map_foreach_entry_const(
            libValPtr,
            mapValueForEachLibFunc<ConstValue, const bt_value,
                                   bt_value_map_foreach_entry_const_func_status,
                                   BT_VALUE_MAP_FOREACH_ENTRY_CONST_FUNC_STATUS_OK,
                                   BT_VALUE_MAP_FOREACH_ENTRY_CONST_FUNC_STATUS_ERROR>,
            const_cast<void *>(reinterpret_cast<const void *>(&func)));

        switch (status) {
        case BT_VALUE_MAP_FOREACH_ENTRY_CONST_STATUS_OK:
            return;
        case BT_VALUE_MAP_FOREACH_ENTRY_CONST_STATUS_USER_ERROR:
        case BT_VALUE_MAP_FOREACH_ENTRY_CONST_STATUS_ERROR:
            throw Error {};
        default:
            bt_common_abort();
        }
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonMapValue final : public CommonValue<LibObjT>
{
private:
    using typename CommonValue<LibObjT>::_ThisCommonValue;

public:
    using typename CommonValue<LibObjT>::LibObjPtr;
    using Shared = SharedValue<CommonMapValue<LibObjT>, LibObjT>;

    explicit CommonMapValue(const LibObjPtr libObjPtr) noexcept : _ThisCommonValue {libObjPtr}
    {
        BT_ASSERT_DBG(this->isMap());
    }

    static Shared create()
    {
        const auto libObjPtr = bt_value_map_create();

        internal::validateCreatedObjPtr(libObjPtr);
        return CommonMapValue::Shared::createWithoutRef(libObjPtr);
    }

    template <typename OtherLibObjT>
    CommonMapValue(const CommonMapValue<OtherLibObjT> val) noexcept : _ThisCommonValue {val}
    {
    }

    template <typename OtherLibObjT>
    CommonMapValue<LibObjT> operator=(const CommonMapValue<OtherLibObjT> val) noexcept
    {
        _ThisCommonValue::operator=(val);
        return *this;
    }

    CommonMapValue<const bt_value> asConst() const noexcept
    {
        return CommonMapValue<const bt_value> {*this};
    }

    std::uint64_t length() const noexcept
    {
        return bt_value_map_get_size(this->libObjPtr());
    }

    bool isEmpty() const noexcept
    {
        return this->length() == 0;
    }

    OptionalBorrowedObject<CommonValue<LibObjT>>
    operator[](const bt2c::CStringView key) const noexcept
    {
        return internal::CommonMapValueSpec<LibObjT>::entryByKey(this->libObjPtr(), key);
    }

    bool hasEntry(const bt2c::CStringView key) const noexcept
    {
        return static_cast<bool>(bt_value_map_has_entry(this->libObjPtr(), key));
    }

    CommonMapValue insert(const bt2c::CStringView key, const Value val) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status = bt_value_map_insert_entry(this->libObjPtr(), key, val.libObjPtr());

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const bool rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status =
            bt_value_map_insert_bool_entry(this->libObjPtr(), key, static_cast<bt_bool>(rawVal));

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const std::uint64_t rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status =
            bt_value_map_insert_unsigned_integer_entry(this->libObjPtr(), key, rawVal);

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const std::int64_t rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status =
            bt_value_map_insert_signed_integer_entry(this->libObjPtr(), key, rawVal);

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const double rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status = bt_value_map_insert_real_entry(this->libObjPtr(), key, rawVal);

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const char *rawVal) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

        const auto status = bt_value_map_insert_string_entry(this->libObjPtr(), key, rawVal);

        this->_handleInsertLibStatus(status);
        return *this;
    }

    CommonMapValue insert(const bt2c::CStringView key, const bt2c::CStringView rawVal) const
    {
        return this->insert(key, rawVal.data());
    }

    CommonArrayValue<bt_value> insertEmptyArray(bt2c::CStringView key) const;
    CommonMapValue<bt_value> insertEmptyMap(bt2c::CStringView key) const;

    CommonMapValue
    forEach(const internal::CommonMapValueForEachUserFunc<CommonValue<LibObjT>>& func) const
    {
        internal::CommonMapValueSpec<LibObjT>::forEach(this->libObjPtr(), func);
        return *this;
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }

private:
    void _handleInsertLibStatus(const bt_value_map_insert_entry_status status) const
    {
        if (status == BT_VALUE_MAP_INSERT_ENTRY_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }
    }
};

using MapValue = CommonMapValue<bt_value>;
using ConstMapValue = CommonMapValue<const bt_value>;

namespace internal {

struct MapValueTypeDescr
{
    using Const = ConstMapValue;
    using NonConst = MapValue;
};

template <>
struct TypeDescr<MapValue> : public MapValueTypeDescr
{
};

template <>
struct TypeDescr<ConstMapValue> : public MapValueTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
ArrayValue CommonValue<LibObjT>::appendEmptyArray() const
{
    return this->asArray().appendEmptyArray();
}

template <typename LibObjT>
MapValue CommonValue<LibObjT>::appendEmptyMap() const
{
    return this->asArray().appendEmptyMap();
}

template <typename LibObjT>
ArrayValue CommonValue<LibObjT>::insertEmptyArray(const bt2c::CStringView key) const
{
    return this->asMap().insertEmptyArray(key);
}

template <typename LibObjT>
MapValue CommonValue<LibObjT>::insertEmptyMap(const bt2c::CStringView key) const
{
    return this->asMap().insertEmptyMap(key);
}

template <typename LibObjT>
CommonNullValue<LibObjT> CommonValue<LibObjT>::asNull() const noexcept
{
    BT_ASSERT_DBG(this->isNull());
    return CommonNullValue<LibObjT> {};
}

template <typename LibObjT>
CommonBoolValue<LibObjT> CommonValue<LibObjT>::asBool() const noexcept
{
    return CommonBoolValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonSignedIntegerValue<LibObjT> CommonValue<LibObjT>::asSignedInteger() const noexcept
{
    return CommonSignedIntegerValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonUnsignedIntegerValue<LibObjT> CommonValue<LibObjT>::asUnsignedInteger() const noexcept
{
    return CommonUnsignedIntegerValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonRealValue<LibObjT> CommonValue<LibObjT>::asReal() const noexcept
{
    return CommonRealValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonStringValue<LibObjT> CommonValue<LibObjT>::asString() const noexcept
{
    return CommonStringValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonArrayValue<LibObjT> CommonValue<LibObjT>::asArray() const noexcept
{
    return CommonArrayValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
CommonMapValue<LibObjT> CommonValue<LibObjT>::asMap() const noexcept
{
    return CommonMapValue<LibObjT> {this->libObjPtr()};
}

template <typename LibObjT>
ArrayValue CommonArrayValue<LibObjT>::appendEmptyArray() const
{
    static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

    bt_value *libElemPtr;
    const auto status = bt_value_array_append_empty_array_element(this->libObjPtr(), &libElemPtr);

    this->_handleAppendLibStatus(status);
    return ArrayValue {libElemPtr};
}

template <typename LibObjT>
MapValue CommonArrayValue<LibObjT>::appendEmptyMap() const
{
    static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstArrayValue`.");

    bt_value *libElemPtr;
    const auto status = bt_value_array_append_empty_map_element(this->libObjPtr(), &libElemPtr);

    this->_handleAppendLibStatus(status);
    return MapValue {libElemPtr};
}

template <typename LibObjT>
ArrayValue CommonMapValue<LibObjT>::insertEmptyArray(const bt2c::CStringView key) const
{
    static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

    bt_value *libEntryPtr;
    const auto status = bt_value_map_insert_empty_array_entry(this->libObjPtr(), key, &libEntryPtr);

    this->_handleInsertLibStatus(status);
    return ArrayValue {libEntryPtr};
}

template <typename LibObjT>
MapValue CommonMapValue<LibObjT>::insertEmptyMap(const bt2c::CStringView key) const
{
    static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstMapValue`.");

    bt_value *libEntryPtr;
    const auto status = bt_value_map_insert_empty_map_entry(this->libObjPtr(), key, &libEntryPtr);

    this->_handleInsertLibStatus(status);
    return MapValue {libEntryPtr};
}

inline BoolValue::Shared createValue(const bool rawVal)
{
    return BoolValue::create(rawVal);
}

inline UnsignedIntegerValue::Shared createValue(const std::uint64_t rawVal)
{
    return UnsignedIntegerValue::create(rawVal);
}

inline SignedIntegerValue::Shared createValue(const std::int64_t rawVal)
{
    return SignedIntegerValue::create(rawVal);
}

inline RealValue::Shared createValue(const double rawVal)
{
    return RealValue::create(rawVal);
}

inline StringValue::Shared createValue(const char * const rawVal)
{
    return StringValue::create(rawVal);
}

inline StringValue::Shared createValue(const bt2c::CStringView rawVal)
{
    return StringValue::create(rawVal);
}

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_VALUE_HPP */
