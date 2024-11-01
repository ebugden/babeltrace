/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2016 Philippe Proulx <pproulx@efficios.com>
 */

%include <babeltrace2/trace-ir/trace.h>

%{
#include "native_bt_trace.i.hpp"
%}

int bt_bt2_trace_add_destruction_listener(bt_trace *trace,
		PyObject *py_callable, bt_listener_id *id);
