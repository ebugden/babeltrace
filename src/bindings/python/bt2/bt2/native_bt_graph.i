/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2017 Philippe Proulx <pproulx@efficios.com>
 */

/* Output argument typemap for connection output (always appends) */
%typemap(in, numinputs=0)
	(const bt_connection **)
	(bt_connection *temp_conn = NULL) {
	$1 = &temp_conn;
}

%typemap(argout)
	(const bt_connection **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_connection, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

/* Output argument typemap for component output (always appends) */
%typemap(in, numinputs=0)
	(const bt_component_source **)
	(bt_component_source *temp_comp = NULL) {
	$1 = &temp_comp;
}

%typemap(in, numinputs=0)
	(const bt_component_filter **)
	(bt_component_filter *temp_comp = NULL) {
	$1 = &temp_comp;
}

%typemap(in, numinputs=0)
	(const bt_component_sink **)
	(bt_component_sink *temp_comp = NULL) {
	$1 = &temp_comp;
}

%typemap(argout) (const bt_component_source **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_component_source, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

%typemap(argout) (const bt_component_filter **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_component_filter, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

%typemap(argout) (const bt_component_sink **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_component_sink, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

%include <babeltrace2/graph/graph.h>

/* Helper functions for Python */

%{
#include "native_bt_graph.i.hpp"
%}

PyObject *bt_bt2_graph_add_port_added_listener(struct bt_graph *graph,
		PyObject *py_callable);

bt_graph_add_component_status
bt_bt2_graph_add_source_component(
		bt_graph *graph,
		const bt_component_class_source *component_class,
		const char *name, const bt_value *params,
		PyObject *obj, bt_logging_level log_level,
		const bt_component_source **component);

bt_graph_add_component_status
bt_bt2_graph_add_filter_component(
		bt_graph *graph,
		const bt_component_class_filter *component_class,
		const char *name, const bt_value *params,
		PyObject *obj, bt_logging_level log_level,
		const bt_component_filter **component);

bt_graph_add_component_status
bt_bt2_graph_add_sink_component(
		bt_graph *graph,
		const bt_component_class_sink *component_class,
		const char *name, const bt_value *params,
		PyObject *obj, bt_logging_level log_level,
		const bt_component_sink **component);
