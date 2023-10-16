# SPDX-License-Identifier: MIT
#
# Copyright (c) 2017 Philippe Proulx <pproulx@efficios.com>

import uuid as uuidp

from bt2 import utils as bt2_utils
from bt2 import object as bt2_object
from bt2 import native_bt
from bt2 import user_attributes as bt2_user_attrs


class ClockClassOffset:
    def __init__(self, seconds=0, cycles=0):
        bt2_utils._check_int64(seconds)
        bt2_utils._check_int64(cycles)
        self._seconds = seconds
        self._cycles = cycles

    @property
    def seconds(self):
        return self._seconds

    @property
    def cycles(self):
        return self._cycles

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            # not comparing apples to apples
            return False

        return (self.seconds, self.cycles) == (other.seconds, other.cycles)


class _ClockClassConst(bt2_object._SharedObject, bt2_user_attrs._WithUserAttrsConst):
    @staticmethod
    def _get_ref(ptr):
        native_bt.clock_class_get_ref(ptr)

    @staticmethod
    def _put_ref(ptr):
        native_bt.clock_class_put_ref(ptr)

    @staticmethod
    def _borrow_user_attributes_ptr(ptr):
        return native_bt.clock_class_borrow_user_attributes_const(ptr)

    @property
    def name(self):
        return native_bt.clock_class_get_name(self._ptr)

    @property
    def description(self):
        return native_bt.clock_class_get_description(self._ptr)

    @property
    def frequency(self):
        return native_bt.clock_class_get_frequency(self._ptr)

    @property
    def precision(self):
        precision = native_bt.clock_class_get_precision(self._ptr)
        return precision

    @property
    def offset(self):
        offset_s, offset_cycles = native_bt.clock_class_get_offset(self._ptr)
        return ClockClassOffset(offset_s, offset_cycles)

    @property
    def origin_is_unix_epoch(self):
        return native_bt.clock_class_origin_is_unix_epoch(self._ptr)

    @property
    def uuid(self):
        uuid_bytes = native_bt.clock_class_get_uuid(self._ptr)

        if uuid_bytes is None:
            return

        return uuidp.UUID(bytes=uuid_bytes)

    def cycles_to_ns_from_origin(self, cycles):
        bt2_utils._check_uint64(cycles)
        status, ns = native_bt.clock_class_cycles_to_ns_from_origin(self._ptr, cycles)
        error_msg = "cannot convert clock value to nanoseconds from origin for given clock class"
        bt2_utils._handle_func_status(status, error_msg)
        return ns


class _ClockClass(bt2_user_attrs._WithUserAttrs, _ClockClassConst):
    @staticmethod
    def _borrow_user_attributes_ptr(ptr):
        return native_bt.clock_class_borrow_user_attributes(ptr)

    @staticmethod
    def _set_user_attributes_ptr(obj_ptr, value_ptr):
        native_bt.clock_class_set_user_attributes(obj_ptr, value_ptr)

    def _name(self, name):
        bt2_utils._check_str(name)
        status = native_bt.clock_class_set_name(self._ptr, name)
        bt2_utils._handle_func_status(status, "cannot set clock class object's name")

    _name = property(fset=_name)

    def _description(self, description):
        bt2_utils._check_str(description)
        status = native_bt.clock_class_set_description(self._ptr, description)
        bt2_utils._handle_func_status(
            status, "cannot set clock class object's description"
        )

    _description = property(fset=_description)

    def _frequency(self, frequency):
        bt2_utils._check_uint64(frequency)
        native_bt.clock_class_set_frequency(self._ptr, frequency)

    _frequency = property(fset=_frequency)

    def _precision(self, precision):
        bt2_utils._check_uint64(precision)
        native_bt.clock_class_set_precision(self._ptr, precision)

    _precision = property(fset=_precision)

    def _offset(self, offset):
        bt2_utils._check_type(offset, ClockClassOffset)
        native_bt.clock_class_set_offset(self._ptr, offset.seconds, offset.cycles)

    _offset = property(fset=_offset)

    def _origin_is_unix_epoch(self, origin_is_unix_epoch):
        bt2_utils._check_bool(origin_is_unix_epoch)
        native_bt.clock_class_set_origin_is_unix_epoch(
            self._ptr, int(origin_is_unix_epoch)
        )

    _origin_is_unix_epoch = property(fset=_origin_is_unix_epoch)

    def _uuid(self, uuid):
        bt2_utils._check_type(uuid, uuidp.UUID)
        native_bt.clock_class_set_uuid(self._ptr, uuid.bytes)

    _uuid = property(fset=_uuid)
