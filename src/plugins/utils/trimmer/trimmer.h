/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * BabelTrace - Trace Trimmer Plug-in
 */

#ifndef BABELTRACE_PLUGINS_UTILS_TRIMMER_TRIMMER_H
#define BABELTRACE_PLUGINS_UTILS_TRIMMER_TRIMMER_H

#include "common/macros.h"
#include <babeltrace2/babeltrace.h>

#ifdef __cplusplus
extern "C" {
#endif

bt_component_class_get_supported_mip_versions_method_status
trimmer_supported_mip_versions(
		bt_self_component_class_filter *self_component_class,
		const bt_value *params, void *initialize_method_data,
		bt_logging_level logging_level,
		bt_integer_range_set_unsigned *supported_versions);

void trimmer_finalize(bt_self_component_filter *self_comp);

bt_component_class_initialize_method_status trimmer_init(
		bt_self_component_filter *self_comp,
		bt_self_component_filter_configuration *config,
		const bt_value *params, void *init_data);

bt_message_iterator_class_initialize_method_status trimmer_msg_iter_init(
		bt_self_message_iterator *self_msg_iter,
		bt_self_message_iterator_configuration *config,
		bt_self_component_port_output *port);

bt_message_iterator_class_next_method_status trimmer_msg_iter_next(
		bt_self_message_iterator *self_msg_iter,
		bt_message_array_const msgs, uint64_t capacity,
		uint64_t *count);

void trimmer_msg_iter_finalize(bt_self_message_iterator *self_msg_iter);

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE_PLUGINS_UTILS_TRIMMER_TRIMMER_H */
