# SPDX-License-Identifier: MIT
#
# Copyright (c) 2017 Philippe Proulx <pproulx@efficios.com>


from bt2 import error as bt2_error
from bt2 import logging as bt2_logging
from bt2 import native_bt

# The purpose of this import is to make the typing module easily accessible
# elsewhere, without having to do the try-except everywhere.
try:
    import typing as _typing_mod  # noqa: F401
except ImportError:
    from bt2 import local_typing as _typing_mod  # noqa: F401


class UnknownObject(Exception):
    """
    Raised when a component class handles a query for an object it doesn't
    know about.
    """

    pass


class _OverflowError(bt2_error._Error, OverflowError):
    pass


class TryAgain(Exception):
    pass


class Stop(StopIteration):
    pass


def _check_bool(o):
    if not isinstance(o, bool):
        raise TypeError("'{}' is not a 'bool' object".format(o.__class__.__name__))


def _check_int(o):
    if not isinstance(o, int):
        raise TypeError("'{}' is not an 'int' object".format(o.__class__.__name__))


def _check_float(o):
    if not isinstance(o, float):
        raise TypeError("'{}' is not a 'float' object".format(o.__class__.__name__))


def _check_str(o):
    if not isinstance(o, str):
        raise TypeError("'{}' is not a 'str' object".format(o.__class__.__name__))


def _check_type(o, expected_type):
    if not isinstance(o, expected_type):
        raise TypeError(
            "'{}' is not a '{}' object".format(o.__class__.__name__, expected_type)
        )


def _is_in_int64_range(v):
    assert isinstance(v, int)
    return v >= -(2**63) and v <= (2**63 - 1)


def _is_int64(v):
    if not isinstance(v, int):
        return False

    return _is_in_int64_range(v)


def _is_in_uint64_range(v):
    assert isinstance(v, int)
    return v >= 0 and v <= (2**64 - 1)


def _is_uint64(v):
    if not isinstance(v, int):
        return False

    return _is_in_uint64_range(v)


def _check_int64(v, msg=None):
    _check_int(v)

    if not _is_in_int64_range(v):
        if msg is None:
            msg = "expecting a signed 64-bit integral value"

        msg += " (got {})".format(v)
        raise ValueError(msg)


def _check_uint64(v, msg=None):
    _check_int(v)

    if not _is_in_uint64_range(v):
        if msg is None:
            msg = "expecting an unsigned 64-bit integral value"

        msg += " (got {})".format(v)
        raise ValueError(msg)


def _is_m1ull(v):
    return v == 18446744073709551615


def _is_pow2(v):
    return v != 0 and ((v & (v - 1)) == 0)


def _check_alignment(a):
    _check_uint64(a)

    if not _is_pow2(a):
        raise ValueError("{} is not a power of two".format(a))


def _check_log_level(log_level):
    _check_int(log_level)

    log_levels = (
        bt2_logging.LoggingLevel.TRACE,
        bt2_logging.LoggingLevel.DEBUG,
        bt2_logging.LoggingLevel.INFO,
        bt2_logging.LoggingLevel.WARNING,
        bt2_logging.LoggingLevel.ERROR,
        bt2_logging.LoggingLevel.FATAL,
        bt2_logging.LoggingLevel.NONE,
    )

    if log_level not in log_levels:
        raise ValueError("'{}' is not a valid logging level".format(log_level))


def _handle_func_status(status, msg=None):
    if status == native_bt.__BT_FUNC_STATUS_OK:
        # no error
        return

    if status == native_bt.__BT_FUNC_STATUS_ERROR:
        assert msg is not None
        raise bt2_error._Error(msg)
    elif status == native_bt.__BT_FUNC_STATUS_MEMORY_ERROR:
        assert msg is not None
        raise bt2_error._MemoryError(msg)
    elif status == native_bt.__BT_FUNC_STATUS_END:
        if msg is None:
            raise Stop
        else:
            raise Stop(msg)
    elif status == native_bt.__BT_FUNC_STATUS_AGAIN:
        if msg is None:
            raise TryAgain
        else:
            raise TryAgain(msg)
    elif status == native_bt.__BT_FUNC_STATUS_OVERFLOW_ERROR:
        raise _OverflowError(msg)
    elif status == native_bt.__BT_FUNC_STATUS_UNKNOWN_OBJECT:
        if msg is None:
            raise UnknownObject
        else:
            raise UnknownObject(msg)
    else:
        assert False


class _ListenerHandle:
    def __init__(self, addr):
        self._addr = addr
        self._listener_id = None

    def _set_listener_id(self, listener_id):
        self._listener_id = listener_id

    def _invalidate(self):
        self._listener_id = None
