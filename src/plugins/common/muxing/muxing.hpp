/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019 Francis Deslauriers <francis.deslauriers@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_COMMON_MUXING_MUXING_HPP
#define BABELTRACE_PLUGINS_COMMON_MUXING_MUXING_HPP

#include "cpp-common/bt2/message.hpp"

namespace muxing {

class MessageComparator
{
public:
    explicit MessageComparator(const std::uint64_t graphMipVersion) :
        _mGraphMipVersion {graphMipVersion}
    {
    }

    int compare(bt2::ConstMessage left, bt2::ConstMessage right) const noexcept;

private:
    static int messageTypeWeight(const bt2::MessageType msgType) noexcept;
    static bt2::OptionalBorrowedObject<bt2::ConstStream>
    borrowStream(const bt2::ConstMessage msg) noexcept;

    template <typename ObjT, typename ComparatorT>
    static int compareOptional(ObjT&& left, ObjT&& right, ComparatorT comparator) noexcept;

    static int compareStrings(const bt2c::CStringView left, const bt2c::CStringView right) noexcept;

    template <typename T>
    static int compareLt(const T left, const T right) noexcept;

    static int compareMsgsTypes(const bt2::MessageType left, const bt2::MessageType right) noexcept;
    static int compareUuids(const bt2c::UuidView left, const bt2c::UuidView right) noexcept;
    static int compareOptUuids(const bt2s::optional<const bt2c::UuidView>& left,
                               const bt2s::optional<const bt2c::UuidView>& right) noexcept;
    static int compareIdentities(const bt2::IdentityView& left,
                                 const bt2::IdentityView& right) noexcept;
    static int compareEventClasses(const bt2::ConstEventClass left,
                                   const bt2::ConstEventClass right) noexcept;
    int compareClockClasses(const bt2::ConstClockClass left,
                            const bt2::ConstClockClass right) const noexcept;
    int compareStreamsSameIds(const bt2::ConstStream left,
                              const bt2::ConstStream right) const noexcept;
    static int compareClockSnapshots(const bt2::ConstClockSnapshot left,
                                     const bt2::ConstClockSnapshot right) noexcept;
    int compareMessagesSameType(const bt2::ConstMessage left,
                                const bt2::ConstMessage right) const noexcept;
    static int compareMessages(const bt2::ConstMessage left,
                               const bt2::ConstMessage right) noexcept;

    std::uint64_t _mGraphMipVersion;
};

} /* namespace muxing */

#endif /* BABELTRACE_PLUGINS_COMMON_MUXING_MUXING_HPP */
