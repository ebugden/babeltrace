# SPDX-License-Identifier: MIT
#
# Copyright (c) 2023 EfficiOS, Inc.

import abc

from bt2 import utils as bt2_utils
from bt2 import value as bt2_value


class _WithUserAttrsBase(abc.ABC):
    @staticmethod
    @abc.abstractmethod
    def _borrow_user_attributes_ptr(ptr):
        raise NotImplementedError

    @property
    @abc.abstractmethod
    def _ptr(self):
        raise NotImplementedError


# Mixin class for objects with user attributes (const version).
class _WithUserAttrsConst(_WithUserAttrsBase):
    @property
    def user_attributes(self) -> bt2_value._MapValueConst:
        return bt2_value._MapValueConst._create_from_ptr_and_get_ref(
            self._borrow_user_attributes_ptr(self._ptr)
        )


# Mixin class for objects with user attributes (non-const version).
class _WithUserAttrs(_WithUserAttrsBase, abc.ABC):
    @property
    def user_attributes(self) -> bt2_value.MapValue:
        return bt2_value.MapValue._create_from_ptr_and_get_ref(
            self._borrow_user_attributes_ptr(self._ptr)
        )

    @staticmethod
    @abc.abstractmethod
    def _set_user_attributes_ptr(obj_ptr, value_ptr):
        raise NotImplementedError

    def _user_attributes(self, user_attributes):
        value = bt2_value.create_value(user_attributes)
        bt2_utils._check_type(value, bt2_value.MapValue)
        self._set_user_attributes_ptr(self._ptr, value._ptr)

    _user_attributes = property(fset=_user_attributes)
