/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2017 Philippe Proulx <pproulx@efficios.com>
 */

/* Output argument typemap for plugin output (always appends) */
%typemap(in, numinputs=0)
	(const bt_plugin **)
	(bt_plugin *temp_plugin = NULL) {
	$1 = &temp_plugin;
}

%typemap(argout)
	(const bt_plugin **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_plugin, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

/* Output argument typemap for plugin set output (always appends) */
%typemap(in, numinputs=0)
	(const bt_plugin_set **)
	(bt_plugin_set *temp_plugin_set = NULL) {
	$1 = &temp_plugin_set;
}

%typemap(argout)
	(const bt_plugin_set **) {
	if (*$1) {
		/* SWIG_AppendOutput() steals the created object */
		$result = SWIG_AppendOutput($result,
				SWIG_NewPointerObj(SWIG_as_voidptr(*$1),
					SWIGTYPE_p_bt_plugin_set, 0));
	} else {
		/* SWIG_AppendOutput() steals Py_None */
		Py_INCREF(Py_None);
		$result = SWIG_AppendOutput($result, Py_None);
	}
}

%include <babeltrace2/plugin/plugin-loading.h>

/* Helpers */

%{
#include "native_bt_plugin.i.hpp"
%}

bt_property_availability bt_bt2_plugin_get_version(
		const bt_plugin *plugin, unsigned int *major,
		unsigned int *minor, unsigned int *patch, const char **extra);

bt_plugin_find_status bt_bt2_plugin_find(const char *plugin_name,
		bt_bool find_in_std_env_var, bt_bool find_in_user_dir,
		bt_bool find_in_sys_dir, bt_bool find_in_static,
		bt_bool fail_on_load_error, const bt_plugin **plugin);

bt_plugin_find_all_status bt_bt2_plugin_find_all(bt_bool find_in_std_env_var,
		bt_bool find_in_user_dir, bt_bool find_in_sys_dir,
		bt_bool find_in_static, bt_bool fail_on_load_error,
		const bt_plugin_set **plugin_set);

bt_plugin_find_all_from_file_status bt_bt2_plugin_find_all_from_file(
		const char *path, bt_bool fail_on_load_error,
		const bt_plugin_set **plugin_set);

bt_plugin_find_all_from_dir_status bt_bt2_plugin_find_all_from_dir(
		const char *path, bt_bool recurse, bt_bool fail_on_load_error,
		const bt_plugin_set **plugin_set);
