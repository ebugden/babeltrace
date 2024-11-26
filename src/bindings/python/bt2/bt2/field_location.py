# SPDX-License-Identifier: MIT
#
# Copyright (c) 2024 EfficiOS Inc.

import enum
import collections.abc

from bt2 import object as bt2_object
from bt2 import native_bt, typing_mod

typing = typing_mod._typing_mod


class FieldLocationScope(enum.Enum):
    PACKET_CONTEXT = native_bt.FIELD_LOCATION_SCOPE_PACKET_CONTEXT
    EVENT_COMMON_CONTEXT = native_bt.FIELD_LOCATION_SCOPE_EVENT_COMMON_CONTEXT
    EVENT_SPECIFIC_CONTEXT = native_bt.FIELD_LOCATION_SCOPE_EVENT_SPECIFIC_CONTEXT
    EVENT_PAYLOAD = native_bt.FIELD_LOCATION_SCOPE_EVENT_PAYLOAD


class _FieldLocationConst(bt2_object._SharedObject, collections.abc.Iterable):
    @staticmethod
    def _get_ref(ptr):
        native_bt.field_location_get_ref(ptr)

    @staticmethod
    def _put_ref(ptr):
        native_bt.field_location_put_ref(ptr)

    @property
    def root_scope(self) -> FieldLocationScope:
        return FieldLocationScope(native_bt.field_location_get_root_scope(self._ptr))

    def __len__(self) -> int:
        return native_bt.field_location_get_item_count(self._ptr)

    def __iter__(self) -> typing.Iterator[str]:
        for idx in range(len(self)):
            yield native_bt.field_location_get_item_by_index(self._ptr, idx)
