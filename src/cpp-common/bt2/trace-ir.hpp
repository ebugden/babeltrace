/*
 * Copyright (c) 2020-2021 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP
#define BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP

#include <cstdint>
#include <type_traits>
#include <vector>

#include <babeltrace2/babeltrace.h>

#include "common/macros.h"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/call.hpp"
#include "cpp-common/bt2s/optional.hpp"
#include "cpp-common/bt2s/span.hpp"

#include "borrowed-object.hpp"
#include "clock-class.hpp"
#include "field-class.hpp"
#include "field-location.hpp"
#include "field.hpp"
#include "internal/utils.hpp"
#include "optional-borrowed-object.hpp"
#include "shared-object.hpp"
#include "value.hpp"

namespace bt2 {

template <typename LibObjT>
class CommonEvent;

template <typename LibObjT>
class CommonPacket;

template <typename LibObjT>
class CommonStream;

template <typename LibObjT>
class CommonTrace;

template <typename LibObjT>
class CommonEventClass;

template <typename LibObjT>
class CommonStreamClass;

template <typename LibObjT>
class CommonTraceClass;

namespace internal {

template <typename LibObjT>
struct CommonEventSpec;

/* Functions specific to mutable events */
template <>
struct CommonEventSpec<bt_event> final
{
    static bt_event_class *cls(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_class(libObjPtr);
    }

    static bt_stream *stream(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_stream(libObjPtr);
    }

    static bt_packet *packet(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_packet(libObjPtr);
    }

    static bt_field *payloadField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_payload_field(libObjPtr);
    }

    static bt_field *specificContextField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_specific_context_field(libObjPtr);
    }

    static bt_field *commonContextField(bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_common_context_field(libObjPtr);
    }
};

/* Functions specific to constant events */
template <>
struct CommonEventSpec<const bt_event> final
{
    static const bt_event_class *cls(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_class_const(libObjPtr);
    }

    static const bt_stream *stream(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_stream_const(libObjPtr);
    }

    static const bt_packet *packet(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_packet_const(libObjPtr);
    }

    static const bt_field *payloadField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_payload_field_const(libObjPtr);
    }

    static const bt_field *specificContextField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_specific_context_field_const(libObjPtr);
    }

    static const bt_field *commonContextField(const bt_event * const libObjPtr) noexcept
    {
        return bt_event_borrow_common_context_field_const(libObjPtr);
    }
};

template <typename LibObjT>
using DepStructField = DepType<LibObjT, StructureField, ConstStructureField>;

} /* namespace internal */

template <typename LibObjT>
class CommonEvent final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonEventSpec<LibObjT>;
    using _Packet = internal::DepPacket<LibObjT>;
    using _Stream = internal::DepStream<LibObjT>;
    using _StructureField = internal::DepStructField<LibObjT>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;

    using Class = internal::DepType<LibObjT, CommonEventClass<bt_event_class>,
                                    CommonEventClass<const bt_event_class>>;

    explicit CommonEvent(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonEvent(const CommonEvent<OtherLibObjT> event) noexcept : _ThisBorrowedObject {event}
    {
    }

    template <typename OtherLibObjT>
    CommonEvent<LibObjT> operator=(const CommonEvent<OtherLibObjT> event) noexcept
    {
        _ThisBorrowedObject::operator=(event);
        return *this;
    }

    CommonEvent<const bt_event> asConst() const noexcept
    {
        return CommonEvent<const bt_event> {*this};
    }

    Class cls() const noexcept;
    _Stream stream() const noexcept;
    OptionalBorrowedObject<_Packet> packet() const noexcept;

    OptionalBorrowedObject<_StructureField> payloadField() const noexcept
    {
        return _Spec::payloadField(this->libObjPtr());
    }

    OptionalBorrowedObject<_StructureField> specificContextField() const noexcept
    {
        return _Spec::specificContextField(this->libObjPtr());
    }

    OptionalBorrowedObject<_StructureField> commonContextField() const noexcept
    {
        return _Spec::commonContextField(this->libObjPtr());
    }
};

using Event = CommonEvent<bt_event>;
using ConstEvent = CommonEvent<const bt_event>;

namespace internal {

struct EventTypeDescr
{
    using Const = ConstEvent;
    using NonConst = Event;
};

template <>
struct TypeDescr<Event> : public EventTypeDescr
{
};

template <>
struct TypeDescr<ConstEvent> : public EventTypeDescr
{
};

struct PacketRefFuncs final
{
    static void get(const bt_packet * const libObjPtr) noexcept
    {
        bt_packet_get_ref(libObjPtr);
    }

    static void put(const bt_packet * const libObjPtr) noexcept
    {
        bt_packet_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonPacketSpec;

/* Functions specific to mutable packets */
template <>
struct CommonPacketSpec<bt_packet> final
{
    static bt_stream *stream(bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_stream(libObjPtr);
    }

    static bt_field *contextField(bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_context_field(libObjPtr);
    }
};

/* Functions specific to constant packets */
template <>
struct CommonPacketSpec<const bt_packet> final
{
    static const bt_stream *stream(const bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_stream_const(libObjPtr);
    }

    static const bt_field *contextField(const bt_packet * const libObjPtr) noexcept
    {
        return bt_packet_borrow_context_field_const(libObjPtr);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonPacket final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonPacketSpec<LibObjT>;
    using _Stream = internal::DepStream<LibObjT>;
    using _StructureField = internal::DepStructField<LibObjT>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonPacket, LibObjT, internal::PacketRefFuncs>;

    explicit CommonPacket(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonPacket(const CommonPacket<OtherLibObjT> packet) noexcept : _ThisBorrowedObject {packet}
    {
    }

    template <typename OtherLibObjT>
    CommonPacket operator=(const CommonPacket<OtherLibObjT> packet) noexcept
    {
        _ThisBorrowedObject::operator=(packet);
        return *this;
    }

    CommonPacket<const bt_packet> asConst() const noexcept
    {
        return CommonPacket<const bt_packet> {*this};
    }

    _Stream stream() const noexcept;

    OptionalBorrowedObject<_StructureField> contextField() const noexcept
    {
        return _Spec::contextField(this->libObjPtr());
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using Packet = CommonPacket<bt_packet>;
using ConstPacket = CommonPacket<const bt_packet>;

namespace internal {

struct PacketTypeDescr
{
    using Const = ConstPacket;
    using NonConst = Packet;
};

template <>
struct TypeDescr<Packet> : public PacketTypeDescr
{
};

template <>
struct TypeDescr<ConstPacket> : public PacketTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
OptionalBorrowedObject<typename CommonEvent<LibObjT>::_Packet>
CommonEvent<LibObjT>::packet() const noexcept
{
    return _Spec::packet(this->libObjPtr());
}

namespace internal {

struct StreamRefFuncs final
{
    static void get(const bt_stream * const libObjPtr) noexcept
    {
        bt_stream_get_ref(libObjPtr);
    }

    static void put(const bt_stream * const libObjPtr) noexcept
    {
        bt_stream_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonStreamSpec;

/* Functions specific to mutable streams */
template <>
struct CommonStreamSpec<bt_stream> final
{
    static bt_stream_class *cls(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_class(libObjPtr);
    }

    static bt_trace *trace(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_trace(libObjPtr);
    }

    static bt_value *userAttributes(bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant streams */
template <>
struct CommonStreamSpec<const bt_stream> final
{
    static const bt_stream_class *cls(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_class_const(libObjPtr);
    }

    static const bt_trace *trace(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_trace_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_stream * const libObjPtr) noexcept
    {
        return bt_stream_borrow_user_attributes_const(libObjPtr);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonStream final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonStreamSpec<LibObjT>;
    using _Trace = internal::DepType<LibObjT, CommonTrace<bt_trace>, CommonTrace<const bt_trace>>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonStream, LibObjT, internal::StreamRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    using Class = internal::DepType<LibObjT, CommonStreamClass<bt_stream_class>,
                                    CommonStreamClass<const bt_stream_class>>;

    explicit CommonStream(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonStream(const CommonStream<OtherLibObjT> stream) noexcept : _ThisBorrowedObject {stream}
    {
    }

    template <typename OtherLibObjT>
    CommonStream operator=(const CommonStream<OtherLibObjT> stream) noexcept
    {
        _ThisBorrowedObject::operator=(stream);
        return *this;
    }

    CommonStream<const bt_stream> asConst() const noexcept
    {
        return CommonStream<const bt_stream> {*this};
    }

    Packet::Shared createPacket() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstStream`.");

        const auto libObjPtr = bt_packet_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Packet::Shared::createWithoutRef(libObjPtr);
    }

    Class cls() const noexcept;
    _Trace trace() const noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_stream_get_id(this->libObjPtr());
    }

    CommonStream name(const bt2c::CStringView name) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstStream`.");

        const auto status = bt_stream_set_name(this->libObjPtr(), name);

        if (status == BT_STREAM_SET_NAME_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_stream_get_name(this->libObjPtr());
    }

    template <typename LibValT>
    CommonStream userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstStream`.");

        bt_stream_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using Stream = CommonStream<bt_stream>;
using ConstStream = CommonStream<const bt_stream>;

namespace internal {

struct StreamTypeDescr
{
    using Const = ConstStream;
    using NonConst = Stream;
};

template <>
struct TypeDescr<Stream> : public StreamTypeDescr
{
};

template <>
struct TypeDescr<ConstStream> : public StreamTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
typename CommonEvent<LibObjT>::_Stream CommonEvent<LibObjT>::stream() const noexcept
{
    return _Stream {_Spec::stream(this->libObjPtr())};
}

template <typename LibObjT>
typename CommonPacket<LibObjT>::_Stream CommonPacket<LibObjT>::stream() const noexcept
{
    return _Stream {_Spec::stream(this->libObjPtr())};
}

namespace internal {

struct TraceRefFuncs final
{
    static void get(const bt_trace * const libObjPtr) noexcept
    {
        bt_trace_get_ref(libObjPtr);
    }

    static void put(const bt_trace * const libObjPtr) noexcept
    {
        bt_trace_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonTraceSpec;

/* Functions specific to mutable traces */
template <>
struct CommonTraceSpec<bt_trace> final
{
    static bt_trace_class *cls(bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_class(libObjPtr);
    }

    static bt_stream *streamByIndex(bt_trace * const libObjPtr, const std::uint64_t index) noexcept
    {
        return bt_trace_borrow_stream_by_index(libObjPtr, index);
    }

    static bt_stream *streamById(bt_trace * const libObjPtr, const std::uint64_t id) noexcept
    {
        return bt_trace_borrow_stream_by_id(libObjPtr, id);
    }

    static bt_value *userAttributes(bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant traces */
template <>
struct CommonTraceSpec<const bt_trace> final
{
    static const bt_trace_class *cls(const bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_class_const(libObjPtr);
    }

    static const bt_stream *streamByIndex(const bt_trace * const libObjPtr,
                                          const std::uint64_t index) noexcept
    {
        return bt_trace_borrow_stream_by_index_const(libObjPtr, index);
    }

    static const bt_stream *streamById(const bt_trace * const libObjPtr,
                                       const std::uint64_t id) noexcept
    {
        return bt_trace_borrow_stream_by_id_const(libObjPtr, id);
    }

    static const bt_value *userAttributes(const bt_trace * const libObjPtr) noexcept
    {
        return bt_trace_borrow_user_attributes_const(libObjPtr);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonTrace final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonTraceSpec<LibObjT>;
    using _Stream = internal::DepStream<LibObjT>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonTrace, LibObjT, internal::TraceRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    using Class = internal::DepType<LibObjT, CommonTraceClass<bt_trace_class>,
                                    CommonTraceClass<const bt_trace_class>>;

    struct ConstEnvironmentEntry
    {
        bt2c::CStringView name;
        ConstValue value;
    };

    explicit CommonTrace(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonTrace(const CommonTrace<OtherLibObjT> trace) noexcept : _ThisBorrowedObject {trace}
    {
    }

    template <typename OtherLibObjT>
    CommonTrace operator=(const CommonTrace<OtherLibObjT> trace) noexcept
    {
        _ThisBorrowedObject::operator=(trace);
        return *this;
    }

    CommonTrace<const bt_trace> asConst() const noexcept
    {
        return CommonTrace<const bt_trace> {*this};
    }

    Class cls() const noexcept;

    CommonTrace name(const bt2c::CStringView name) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTrace`.");

        const auto status = bt_trace_set_name(this->libObjPtr(), name);

        if (status == BT_TRACE_SET_NAME_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_trace_get_name(this->libObjPtr());
    }

    CommonTrace uuid(const bt2c::UuidView& uuid) const noexcept
    {
        bt_trace_set_uuid(this->libObjPtr(), uuid.begin());
        return *this;
    }

    bt2s::optional<bt2c::UuidView> uuid() const noexcept
    {
        const auto uuid = bt_trace_get_uuid(this->libObjPtr());

        if (uuid) {
            return bt2c::UuidView {uuid};
        }

        return bt2s::nullopt;
    }

    std::uint64_t length() const noexcept
    {
        return bt_trace_get_stream_count(this->libObjPtr());
    }

    _Stream operator[](const std::uint64_t index) const noexcept
    {
        return _Stream {_Spec::streamByIndex(this->libObjPtr(), index)};
    }

    OptionalBorrowedObject<_Stream> streamById(const std::uint64_t id) const noexcept
    {
        return _Spec::streamById(this->libObjPtr(), id);
    }

    CommonTrace environmentEntry(const bt2c::CStringView name, const std::int64_t val) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTrace`.");

        const auto status = bt_trace_set_environment_entry_integer(this->libObjPtr(), name, val);

        if (status == BT_TRACE_SET_ENVIRONMENT_ENTRY_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    CommonTrace environmentEntry(const bt2c::CStringView name, const bt2c::CStringView val) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTrace`.");

        const auto status = bt_trace_set_environment_entry_string(this->libObjPtr(), name, val);

        if (status == BT_TRACE_SET_ENVIRONMENT_ENTRY_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    std::uint64_t environmentSize() const noexcept
    {
        return bt_trace_get_environment_entry_count(this->libObjPtr());
    }

    ConstEnvironmentEntry environmentEntry(const std::uint64_t index) const noexcept
    {
        const char *name;
        const bt_value *libObjPtr;

        bt_trace_borrow_environment_entry_by_index_const(this->libObjPtr(), index, &name,
                                                         &libObjPtr);
        return ConstEnvironmentEntry {name, ConstValue {libObjPtr}};
    }

    OptionalBorrowedObject<ConstValue> environmentEntry(const bt2c::CStringView name) const noexcept
    {
        return bt_trace_borrow_environment_entry_value_by_name_const(this->libObjPtr(), name);
    }

    template <typename LibValT>
    CommonTrace userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTrace`.");

        bt_trace_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using Trace = CommonTrace<bt_trace>;
using ConstTrace = CommonTrace<const bt_trace>;

namespace internal {

struct TraceTypeDescr
{
    using Const = ConstTrace;
    using NonConst = Trace;
};

template <>
struct TypeDescr<Trace> : public TraceTypeDescr
{
};

template <>
struct TypeDescr<ConstTrace> : public TraceTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
typename CommonStream<LibObjT>::_Trace CommonStream<LibObjT>::trace() const noexcept
{
    return _Trace {_Spec::trace(this->libObjPtr())};
}

namespace internal {

struct EventClassRefFuncs final
{
    static void get(const bt_event_class * const libObjPtr) noexcept
    {
        bt_event_class_get_ref(libObjPtr);
    }

    static void put(const bt_event_class * const libObjPtr) noexcept
    {
        bt_event_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonEventClassSpec;

/* Functions specific to mutable event classes */
template <>
struct CommonEventClassSpec<bt_event_class> final
{
    static bt_stream_class *streamClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_stream_class(libObjPtr);
    }

    static bt_field_class *payloadFieldClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_payload_field_class(libObjPtr);
    }

    static bt_field_class *specificContextFieldClass(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_specific_context_field_class(libObjPtr);
    }

    static bt_value *userAttributes(bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant event classes */
template <>
struct CommonEventClassSpec<const bt_event_class> final
{
    static const bt_stream_class *streamClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_stream_class_const(libObjPtr);
    }

    static const bt_field_class *payloadFieldClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_payload_field_class_const(libObjPtr);
    }

    static const bt_field_class *
    specificContextFieldClass(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_specific_context_field_class_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_event_class * const libObjPtr) noexcept
    {
        return bt_event_class_borrow_user_attributes_const(libObjPtr);
    }
};

template <typename LibObjT>
using DepStructFc = DepType<LibObjT, StructureFieldClass, ConstStructureFieldClass>;

} /* namespace internal */

/* Avoid `-Wshadow` error on GCC, conflicting with `bt2::Error` */
BT_DIAG_PUSH
BT_DIAG_IGNORE_SHADOW

enum class EventClassLogLevel
{
    Emergency = BT_EVENT_CLASS_LOG_LEVEL_EMERGENCY,
    Alert = BT_EVENT_CLASS_LOG_LEVEL_ALERT,
    Critical = BT_EVENT_CLASS_LOG_LEVEL_CRITICAL,
    Error = BT_EVENT_CLASS_LOG_LEVEL_ERROR,
    Warning = BT_EVENT_CLASS_LOG_LEVEL_WARNING,
    Notice = BT_EVENT_CLASS_LOG_LEVEL_NOTICE,
    Info = BT_EVENT_CLASS_LOG_LEVEL_INFO,
    DebugSystem = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_SYSTEM,
    DebugProgram = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_PROGRAM,
    DebugProcess = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_PROCESS,
    DebugModule = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_MODULE,
    DebugUnit = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_UNIT,
    DebugFunction = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_FUNCTION,
    DebugLine = BT_EVENT_CLASS_LOG_LEVEL_DEBUG_LINE,
    Debug = BT_EVENT_CLASS_LOG_LEVEL_DEBUG,
};

BT_DIAG_POP

template <typename LibObjT>
class CommonEventClass final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonEventClassSpec<LibObjT>;
    using _StructureFieldClass = internal::DepStructFc<LibObjT>;

    using _StreamClass = internal::DepType<LibObjT, CommonStreamClass<bt_stream_class>,
                                           CommonStreamClass<const bt_stream_class>>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonEventClass, LibObjT, internal::EventClassRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    explicit CommonEventClass(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonEventClass(const CommonEventClass<OtherLibObjT> eventClass) noexcept :
        _ThisBorrowedObject {eventClass}
    {
    }

    template <typename OtherLibObjT>
    CommonEventClass operator=(const CommonEventClass<OtherLibObjT> eventClass) noexcept
    {
        _ThisBorrowedObject::operator=(eventClass);
        return *this;
    }

    CommonEventClass<const bt_event_class> asConst() const noexcept
    {
        return CommonEventClass<const bt_event_class> {*this};
    }

    _StreamClass streamClass() const noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_event_class_get_id(this->libObjPtr());
    }

    CommonEventClass nameSpace(const bt2c::CStringView nameSpace) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        if (bt_event_class_set_namespace(this->libObjPtr(), nameSpace) ==
            BT_EVENT_CLASS_SET_NAMESPACE_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView nameSpace() const noexcept
    {
        return bt_event_class_get_namespace(this->libObjPtr());
    }

    CommonEventClass name(const bt2c::CStringView name) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        const auto status = bt_event_class_set_name(this->libObjPtr(), name);

        if (status == BT_EVENT_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_event_class_get_name(this->libObjPtr());
    }

    CommonEventClass logLevel(const EventClassLogLevel logLevel) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        bt_event_class_set_log_level(this->libObjPtr(),
                                     static_cast<bt_event_class_log_level>(logLevel));
        return *this;
    }

    bt2s::optional<EventClassLogLevel> logLevel() const noexcept
    {
        bt_event_class_log_level libLogLevel;

        if (bt_event_class_get_log_level(this->libObjPtr(), &libLogLevel)) {
            return static_cast<EventClassLogLevel>(libLogLevel);
        }

        return bt2s::nullopt;
    }

    CommonEventClass emfUri(const bt2c::CStringView emfUri) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        const auto status = bt_event_class_set_emf_uri(this->libObjPtr(), emfUri);

        if (status == BT_EVENT_CLASS_SET_EMF_URI_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView emfUri() const noexcept
    {
        return bt_event_class_get_emf_uri(this->libObjPtr());
    }

    CommonEventClass payloadFieldClass(const StructureFieldClass fc) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        const auto status =
            bt_event_class_set_payload_field_class(this->libObjPtr(), fc.libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    OptionalBorrowedObject<_StructureFieldClass> payloadFieldClass() const noexcept
    {
        return _Spec::payloadFieldClass(this->libObjPtr());
    }

    CommonEventClass specificContextFieldClass(const StructureFieldClass fc) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        const auto status =
            bt_event_class_set_specific_context_field_class(this->libObjPtr(), fc.libObjPtr());

        if (status == BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    OptionalBorrowedObject<_StructureFieldClass> specificContextFieldClass() const noexcept
    {
        return _Spec::specificContextFieldClass(this->libObjPtr());
    }

    template <typename LibValT>
    CommonEventClass userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstEventClass`.");

        bt_event_class_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using EventClass = CommonEventClass<bt_event_class>;
using ConstEventClass = CommonEventClass<const bt_event_class>;

namespace internal {

struct EventClassTypeDescr
{
    using Const = ConstEventClass;
    using NonConst = EventClass;
};

template <>
struct TypeDescr<EventClass> : public EventClassTypeDescr
{
};

template <>
struct TypeDescr<ConstEventClass> : public EventClassTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
typename CommonEvent<LibObjT>::Class CommonEvent<LibObjT>::cls() const noexcept
{
    return Class {_Spec::cls(this->libObjPtr())};
}

namespace internal {

struct StreamClassRefFuncs final
{
    static void get(const bt_stream_class * const libObjPtr) noexcept
    {
        bt_stream_class_get_ref(libObjPtr);
    }

    static void put(const bt_stream_class * const libObjPtr) noexcept
    {
        bt_stream_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonStreamClassSpec;

/* Functions specific to mutable stream classes */
template <>
struct CommonStreamClassSpec<bt_stream_class> final
{
    static bt_trace_class *traceClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_trace_class(libObjPtr);
    }

    static bt_event_class *eventClassByIndex(bt_stream_class * const libObjPtr,
                                             const std::uint64_t index) noexcept
    {
        return bt_stream_class_borrow_event_class_by_index(libObjPtr, index);
    }

    static bt_event_class *eventClassById(bt_stream_class * const libObjPtr,
                                          const std::uint64_t id) noexcept
    {
        return bt_stream_class_borrow_event_class_by_id(libObjPtr, id);
    }

    static bt_clock_class *defaultClockClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_default_clock_class(libObjPtr);
    }

    static bt_field_class *packetContextFieldClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_packet_context_field_class(libObjPtr);
    }

    static bt_field_class *eventCommonContextFieldClass(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_event_common_context_field_class(libObjPtr);
    }

    static bt_value *userAttributes(bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant stream classes */
template <>
struct CommonStreamClassSpec<const bt_stream_class> final
{
    static const bt_trace_class *traceClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_trace_class_const(libObjPtr);
    }

    static const bt_event_class *eventClassByIndex(const bt_stream_class * const libObjPtr,
                                                   const std::uint64_t index) noexcept
    {
        return bt_stream_class_borrow_event_class_by_index_const(libObjPtr, index);
    }

    static const bt_event_class *eventClassById(const bt_stream_class * const libObjPtr,
                                                const std::uint64_t id) noexcept
    {
        return bt_stream_class_borrow_event_class_by_id_const(libObjPtr, id);
    }

    static const bt_clock_class *defaultClockClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_default_clock_class_const(libObjPtr);
    }

    static const bt_field_class *
    packetContextFieldClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_packet_context_field_class_const(libObjPtr);
    }

    static const bt_field_class *
    eventCommonContextFieldClass(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_event_common_context_field_class_const(libObjPtr);
    }

    static const bt_value *userAttributes(const bt_stream_class * const libObjPtr) noexcept
    {
        return bt_stream_class_borrow_user_attributes_const(libObjPtr);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonStreamClass final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;
    using _Spec = internal::CommonStreamClassSpec<LibObjT>;
    using _StructureFieldClass = internal::DepStructFc<LibObjT>;

    using _TraceClass = internal::DepType<LibObjT, CommonTraceClass<bt_trace_class>,
                                          CommonTraceClass<const bt_trace_class>>;

    using _EventClass = internal::DepType<LibObjT, CommonEventClass<bt_event_class>,
                                          CommonEventClass<const bt_event_class>>;

    using _ClockClass = internal::DepType<LibObjT, ClockClass, ConstClockClass>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonStreamClass, LibObjT, internal::StreamClassRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    explicit CommonStreamClass(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonStreamClass(const CommonStreamClass<OtherLibObjT> streamClass) noexcept :
        _ThisBorrowedObject {streamClass}
    {
    }

    template <typename OtherLibObjT>
    CommonStreamClass operator=(const CommonStreamClass<OtherLibObjT> streamClass) noexcept
    {
        _ThisBorrowedObject::operator=(streamClass);
        return *this;
    }

    CommonStreamClass<const bt_stream_class> asConst() const noexcept
    {
        return CommonStreamClass<const bt_stream_class> {*this};
    }

    Stream::Shared instantiate(const Trace trace) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto libObjPtr = bt_stream_create(this->libObjPtr(), trace.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Stream::Shared::createWithoutRef(libObjPtr);
    }

    Stream::Shared instantiate(const Trace trace, const std::uint64_t id) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto libObjPtr = bt_stream_create_with_id(this->libObjPtr(), trace.libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return Stream::Shared::createWithoutRef(libObjPtr);
    }

    EventClass::Shared createEventClass() const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto libObjPtr = bt_event_class_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return EventClass::Shared::createWithoutRef(libObjPtr);
    }

    EventClass::Shared createEventClass(const std::uint64_t id) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto libObjPtr = bt_event_class_create_with_id(this->libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return EventClass::Shared::createWithoutRef(libObjPtr);
    }

    _TraceClass traceClass() const noexcept;

    std::uint64_t id() const noexcept
    {
        return bt_stream_class_get_id(this->libObjPtr());
    }

    CommonStreamClass nameSpace(const bt2c::CStringView nameSpace) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        if (bt_stream_class_set_namespace(this->libObjPtr(), nameSpace) ==
            BT_STREAM_CLASS_SET_NAMESPACE_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView nameSpace() const noexcept
    {
        return bt_stream_class_get_namespace(this->libObjPtr());
    }

    CommonStreamClass name(const bt2c::CStringView name) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto status = bt_stream_class_set_name(this->libObjPtr(), name);

        if (status == BT_STREAM_CLASS_SET_NAME_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_stream_class_get_name(this->libObjPtr());
    }

    CommonStreamClass assignsAutomaticEventClassId(const bool val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_assigns_automatic_event_class_id(this->libObjPtr(),
                                                             static_cast<bt_bool>(val));
        return *this;
    }

    bool assignsAutomaticEventClassId() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_assigns_automatic_event_class_id(this->libObjPtr()));
    }

    CommonStreamClass assignsAutomaticStreamId(const bool val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_assigns_automatic_stream_id(this->libObjPtr(),
                                                        static_cast<bt_bool>(val));
        return *this;
    }

    bool assignsAutomaticStreamId() const noexcept
    {
        return static_cast<bool>(bt_stream_class_assigns_automatic_stream_id(this->libObjPtr()));
    }

    CommonStreamClass supportsPackets(const bool supportsPackets,
                                      const bool withBeginningDefaultClkSnapshot,
                                      const bool withEndDefaultClkSnapshot) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_supports_packets(this->libObjPtr(),
                                             static_cast<bt_bool>(supportsPackets),
                                             static_cast<bt_bool>(withBeginningDefaultClkSnapshot),
                                             static_cast<bt_bool>(withEndDefaultClkSnapshot));
        return *this;
    }

    bool supportsPackets() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_packets(this->libObjPtr()));
    }

    bool packetsHaveBeginningClockSnapshot() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_packets_have_beginning_default_clock_snapshot(this->libObjPtr()));
    }

    bool packetsHaveEndClockSnapshot() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_packets_have_end_default_clock_snapshot(this->libObjPtr()));
    }

    CommonStreamClass supportsDiscardedEvents(const bool supportsDiscardedEvents,
                                              const bool withDefaultClkSnapshots) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_supports_discarded_events(
            this->libObjPtr(), static_cast<bt_bool>(supportsDiscardedEvents),
            static_cast<bt_bool>(withDefaultClkSnapshots));
        return *this;
    }

    bool supportsDiscardedEvents() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_discarded_events(this->libObjPtr()));
    }

    bool discardedEventsHaveDefaultClockSnapshots() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_discarded_events_have_default_clock_snapshots(this->libObjPtr()));
    }

    CommonStreamClass supportsDiscardedPackets(const bool supportsDiscardedPackets,
                                               const bool withDefaultClkSnapshots) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_supports_discarded_packets(
            this->libObjPtr(), static_cast<bt_bool>(supportsDiscardedPackets),
            static_cast<bt_bool>(withDefaultClkSnapshots));
        return *this;
    }

    bool supportsDiscardedPackets() const noexcept
    {
        return static_cast<bool>(bt_stream_class_supports_discarded_packets(this->libObjPtr()));
    }

    bool discardedPacketsHaveDefaultClockSnapshots() const noexcept
    {
        return static_cast<bool>(
            bt_stream_class_discarded_packets_have_default_clock_snapshots(this->libObjPtr()));
    }

    CommonStreamClass defaultClockClass(const ClockClass clkCls) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto status =
            bt_stream_class_set_default_clock_class(this->libObjPtr(), clkCls.libObjPtr());

        BT_ASSERT(status == BT_STREAM_CLASS_SET_DEFAULT_CLOCK_CLASS_STATUS_OK);
        return *this;
    }

    OptionalBorrowedObject<_ClockClass> defaultClockClass() const noexcept
    {
        return _Spec::defaultClockClass(this->libObjPtr());
    }

    std::uint64_t length() const noexcept
    {
        return bt_stream_class_get_event_class_count(this->libObjPtr());
    }

    _EventClass operator[](const std::uint64_t index) const noexcept
    {
        return _EventClass {_Spec::eventClassByIndex(this->libObjPtr(), index)};
    }

    OptionalBorrowedObject<_EventClass> eventClassById(const std::uint64_t id) const noexcept
    {
        return _Spec::eventClassById(this->libObjPtr(), id);
    }

    CommonStreamClass packetContextFieldClass(const StructureFieldClass fc) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto status =
            bt_stream_class_set_packet_context_field_class(this->libObjPtr(), fc.libObjPtr());

        if (status == BT_STREAM_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    OptionalBorrowedObject<_StructureFieldClass> packetContextFieldClass() const noexcept
    {
        return _Spec::packetContextFieldClass(this->libObjPtr());
    }

    CommonStreamClass eventCommonContextFieldClass(const StructureFieldClass fc) const
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        const auto status =
            bt_stream_class_set_event_common_context_field_class(this->libObjPtr(), fc.libObjPtr());

        if (status == BT_STREAM_CLASS_SET_FIELD_CLASS_STATUS_MEMORY_ERROR) {
            throw MemoryError {};
        }

        return *this;
    }

    OptionalBorrowedObject<_StructureFieldClass> eventCommonContextFieldClass() const noexcept
    {
        return _Spec::eventCommonContextFieldClass(this->libObjPtr());
    }

    template <typename LibValT>
    CommonStreamClass userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value,
                      "Not available with `bt2::ConstStreamClass`.");

        bt_stream_class_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }
};

using StreamClass = CommonStreamClass<bt_stream_class>;
using ConstStreamClass = CommonStreamClass<const bt_stream_class>;

namespace internal {

struct StreamClassTypeDescr
{
    using Const = ConstStreamClass;
    using NonConst = StreamClass;
};

template <>
struct TypeDescr<StreamClass> : public StreamClassTypeDescr
{
};

template <>
struct TypeDescr<ConstStreamClass> : public StreamClassTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
typename CommonEventClass<LibObjT>::_StreamClass
CommonEventClass<LibObjT>::streamClass() const noexcept
{
    return _StreamClass {_Spec::streamClass(this->libObjPtr())};
}

template <typename LibObjT>
typename CommonStream<LibObjT>::Class CommonStream<LibObjT>::cls() const noexcept
{
    return Class {_Spec::cls(this->libObjPtr())};
}

namespace internal {

struct TraceClassRefFuncs final
{
    static void get(const bt_trace_class * const libObjPtr) noexcept
    {
        bt_trace_class_get_ref(libObjPtr);
    }

    static void put(const bt_trace_class * const libObjPtr) noexcept
    {
        bt_trace_class_put_ref(libObjPtr);
    }
};

template <typename LibObjT>
struct CommonTraceClassSpec;

/* Functions specific to mutable stream classes */
template <>
struct CommonTraceClassSpec<bt_trace_class> final
{
    static bt_stream_class *streamClassByIndex(bt_trace_class * const libObjPtr,
                                               const std::uint64_t index) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_index(libObjPtr, index);
    }

    static bt_stream_class *streamClassById(bt_trace_class * const libObjPtr,
                                            const std::uint64_t id) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_id(libObjPtr, id);
    }

    static bt_value *userAttributes(bt_trace_class * const libObjPtr) noexcept
    {
        return bt_trace_class_borrow_user_attributes(libObjPtr);
    }
};

/* Functions specific to constant stream classes */
template <>
struct CommonTraceClassSpec<const bt_trace_class> final
{
    static const bt_stream_class *streamClassByIndex(const bt_trace_class * const libObjPtr,
                                                     const std::uint64_t index) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_index_const(libObjPtr, index);
    }

    static const bt_stream_class *streamClassById(const bt_trace_class * const libObjPtr,
                                                  const std::uint64_t id) noexcept
    {
        return bt_trace_class_borrow_stream_class_by_id_const(libObjPtr, id);
    }

    static const bt_value *userAttributes(const bt_trace_class * const libObjPtr) noexcept
    {
        return bt_trace_class_borrow_user_attributes_const(libObjPtr);
    }
};

} /* namespace internal */

template <typename LibObjT>
class CommonTraceClass final : public BorrowedObject<LibObjT>
{
private:
    using typename BorrowedObject<LibObjT>::_ThisBorrowedObject;

    using _Spec = internal::CommonTraceClassSpec<LibObjT>;

    using _StreamClass = internal::DepType<LibObjT, CommonStreamClass<bt_stream_class>,
                                           CommonStreamClass<const bt_stream_class>>;

public:
    using typename BorrowedObject<LibObjT>::LibObjPtr;
    using Shared = SharedObject<CommonTraceClass, LibObjT, internal::TraceClassRefFuncs>;
    using UserAttributes = internal::DepUserAttrs<LibObjT>;

    explicit CommonTraceClass(const LibObjPtr libObjPtr) noexcept : _ThisBorrowedObject {libObjPtr}
    {
    }

    template <typename OtherLibObjT>
    CommonTraceClass(const CommonTraceClass<OtherLibObjT> traceClass) noexcept :
        _ThisBorrowedObject {traceClass}
    {
    }

    template <typename OtherLibObjT>
    CommonTraceClass operator=(const CommonTraceClass<OtherLibObjT> traceClass) noexcept
    {
        _ThisBorrowedObject::operator=(traceClass);
        return *this;
    }

    CommonTraceClass<const bt_trace_class> asConst() const noexcept
    {
        return CommonTraceClass<const bt_trace_class> {*this};
    }

    Trace::Shared instantiate() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_trace_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return Trace::Shared::createWithoutRef(libObjPtr);
    }

    ConstFieldLocation::Shared createFieldLocation(const ConstFieldLocation::Scope scope,
                                                   const bt2s::span<const char * const> items) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr =
            bt_field_location_create(this->libObjPtr(), static_cast<bt_field_location_scope>(scope),
                                     items.data(), items.size());

        internal::validateCreatedObjPtr(libObjPtr);
        return ConstFieldLocation::Shared::createWithoutRef(libObjPtr);
    }

    ConstFieldLocation::Shared createFieldLocation(const ConstFieldLocation::Scope scope,
                                                   const bt2s::span<const std::string> items) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto ptrItems = bt2c::call([items] {
            std::vector<const char *> v;

            for (auto& item : items) {
                v.push_back(item.data());
            }

            return v;
        });

        return this->createFieldLocation(scope, ptrItems);
    }

    StreamClass::Shared createStreamClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_stream_class_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return StreamClass::Shared::createWithoutRef(libObjPtr);
    }

    StreamClass::Shared createStreamClass(const std::uint64_t id) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_stream_class_create_with_id(this->libObjPtr(), id);

        internal::validateCreatedObjPtr(libObjPtr);
        return StreamClass::Shared::createWithoutRef(libObjPtr);
    }

    FieldClass::Shared createBoolFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_bool_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared::createWithoutRef(libObjPtr);
    }

    BitArrayFieldClass::Shared createBitArrayFieldClass(const std::uint64_t length) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_bit_array_create(this->libObjPtr(), length);

        internal::validateCreatedObjPtr(libObjPtr);
        return BitArrayFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    IntegerFieldClass::Shared createUnsignedIntegerFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_integer_unsigned_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return IntegerFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    IntegerFieldClass::Shared createSignedIntegerFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_integer_signed_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return IntegerFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    UnsignedEnumerationFieldClass::Shared createUnsignedEnumerationFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_enumeration_unsigned_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return UnsignedEnumerationFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    SignedEnumerationFieldClass::Shared createSignedEnumerationFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_enumeration_signed_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return SignedEnumerationFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    FieldClass::Shared createSinglePrecisionRealFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_real_single_precision_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared::createWithoutRef(libObjPtr);
    }

    FieldClass::Shared createDoublePrecisionRealFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_real_double_precision_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared::createWithoutRef(libObjPtr);
    }

    FieldClass::Shared createStringFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_string_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return FieldClass::Shared::createWithoutRef(libObjPtr);
    }

    StaticArrayFieldClass::Shared createStaticArrayFieldClass(const FieldClass elementFieldClass,
                                                              const std::uint64_t length) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_array_static_create(
            this->libObjPtr(), elementFieldClass.libObjPtr(), length);

        internal::validateCreatedObjPtr(libObjPtr);
        return StaticArrayFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    ArrayFieldClass::Shared createDynamicArrayFieldClass(const FieldClass elementFieldClass) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_array_dynamic_create(
            this->libObjPtr(), elementFieldClass.libObjPtr(), nullptr);

        internal::validateCreatedObjPtr(libObjPtr);
        return ArrayFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    DynamicArrayWithLengthFieldClass::Shared
    createDynamicArrayFieldClass(const FieldClass elementFieldClass,
                                 const IntegerFieldClass lengthFieldClass) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_array_dynamic_create(
            this->libObjPtr(), elementFieldClass.libObjPtr(), lengthFieldClass.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return DynamicArrayWithLengthFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    StructureFieldClass::Shared createStructureFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_structure_create(this->libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return StructureFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    OptionFieldClass::Shared createOptionFieldClass(const FieldClass optionalFieldClass) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_option_without_selector_create(
            this->libObjPtr(), optionalFieldClass.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    OptionWithBoolSelectorFieldClass::Shared
    createOptionWithBoolSelectorFieldClass(const FieldClass optionalFieldClass,
                                           const FieldClass selectorFieldClass) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_bool_create(
            this->libObjPtr(), optionalFieldClass.libObjPtr(), selectorFieldClass.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithBoolSelectorFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    OptionWithUnsignedIntegerSelectorFieldClass::Shared
    createOptionWithUnsignedIntegerSelectorFieldClass(
        const FieldClass optionalFieldClass, const IntegerFieldClass selectorFieldClass,
        const ConstUnsignedIntegerRangeSet ranges) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_integer_unsigned_create(
            this->libObjPtr(), optionalFieldClass.libObjPtr(), selectorFieldClass.libObjPtr(),
            ranges.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithUnsignedIntegerSelectorFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    OptionWithSignedIntegerSelectorFieldClass::Shared
    createOptionWithSignedIntegerSelectorFieldClass(const FieldClass optionalFieldClass,
                                                    const IntegerFieldClass selectorFieldClass,
                                                    const ConstSignedIntegerRangeSet ranges) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_option_with_selector_field_integer_signed_create(
            this->libObjPtr(), optionalFieldClass.libObjPtr(), selectorFieldClass.libObjPtr(),
            ranges.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return OptionWithSignedIntegerSelectorFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    VariantWithoutSelectorFieldClass::Shared createVariantFieldClass() const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr = bt_field_class_variant_create(this->libObjPtr(), nullptr);

        internal::validateCreatedObjPtr(libObjPtr);
        return VariantWithoutSelectorFieldClass::Shared::createWithoutRef(libObjPtr);
    }

    VariantWithUnsignedIntegerSelectorFieldClass::Shared
    createVariantWithUnsignedIntegerSelectorFieldClass(
        const IntegerFieldClass selectorFieldClass) const
    {
        return this->_createVariantWithIntegerSelectorFieldClass<
            VariantWithUnsignedIntegerSelectorFieldClass>(selectorFieldClass);
    }

    VariantWithSignedIntegerSelectorFieldClass::Shared
    createVariantWithSignedIntegerSelectorFieldClass(
        const IntegerFieldClass selectorFieldClass) const
    {
        return this->_createVariantWithIntegerSelectorFieldClass<
            VariantWithSignedIntegerSelectorFieldClass>(selectorFieldClass);
    }

    CommonTraceClass assignsAutomaticStreamClassId(const bool val) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        bt_trace_class_set_assigns_automatic_stream_class_id(this->libObjPtr(),
                                                             static_cast<bt_bool>(val));
        return *this;
    }

    bool assignsAutomaticStreamClassId() const noexcept
    {
        return static_cast<bool>(
            bt_trace_class_assigns_automatic_stream_class_id(this->libObjPtr()));
    }

    std::uint64_t length() const noexcept
    {
        return bt_trace_class_get_stream_class_count(this->libObjPtr());
    }

    _StreamClass operator[](const std::uint64_t index) const noexcept
    {
        return _StreamClass {_Spec::streamClassByIndex(this->libObjPtr(), index)};
    }

    OptionalBorrowedObject<_StreamClass> streamClassById(const std::uint64_t id) const noexcept
    {
        return _Spec::streamClassById(this->libObjPtr(), id);
    }

    template <typename LibValT>
    CommonTraceClass userAttributes(const CommonMapValue<LibValT> userAttrs) const noexcept
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        bt_trace_class_set_user_attributes(this->libObjPtr(), userAttrs.libObjPtr());
        return *this;
    }

    UserAttributes userAttributes() const noexcept
    {
        return UserAttributes {_Spec::userAttributes(this->libObjPtr())};
    }

    Shared shared() const noexcept
    {
        return Shared::createWithRef(*this);
    }

private:
    template <typename ObjT>
    typename ObjT::Shared
    _createVariantWithIntegerSelectorFieldClass(const IntegerFieldClass selectorFieldClass) const
    {
        static_assert(!std::is_const<LibObjT>::value, "Not available with `bt2::ConstTraceClass`.");

        const auto libObjPtr =
            bt_field_class_variant_create(this->libObjPtr(), selectorFieldClass.libObjPtr());

        internal::validateCreatedObjPtr(libObjPtr);
        return ObjT::Shared::createWithoutRef(libObjPtr);
    }
};

using TraceClass = CommonTraceClass<bt_trace_class>;
using ConstTraceClass = CommonTraceClass<const bt_trace_class>;

namespace internal {

struct TraceClassTypeDescr
{
    using Const = ConstTraceClass;
    using NonConst = TraceClass;
};

template <>
struct TypeDescr<TraceClass> : public TraceClassTypeDescr
{
};

template <>
struct TypeDescr<ConstTraceClass> : public TraceClassTypeDescr
{
};

} /* namespace internal */

template <typename LibObjT>
typename CommonStreamClass<LibObjT>::_TraceClass
CommonStreamClass<LibObjT>::traceClass() const noexcept
{
    return _TraceClass {_Spec::traceClass(this->libObjPtr())};
}

template <typename LibObjT>
typename CommonTrace<LibObjT>::Class CommonTrace<LibObjT>::cls() const noexcept
{
    return Class {_Spec::cls(this->libObjPtr())};
}

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_TRACE_IR_HPP */
