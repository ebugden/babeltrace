/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2019 EfficiOS Inc. and Linux Foundation
 * Copyright (c) 2019 Francis Deslauriers <francis.deslauriers@efficios.com>
 *
 * Babeltrace - Trace IR data object copy
 */

#ifndef BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_DATA_COPY_H
#define BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_DATA_COPY_H

#include <babeltrace2/babeltrace.h>

#include "common/macros.h"
#include "trace-ir-mapping.hpp"

enum debug_info_trace_ir_mapping_status copy_trace_content(
		const bt_trace *in_trace, bt_trace *out_trace,
		bt_logging_level log_level, bt_self_component *self_comp);
enum debug_info_trace_ir_mapping_status copy_stream_content(
		const bt_stream *in_stream, bt_stream *out_stream,
		bt_logging_level log_level, bt_self_component *self_comp);
enum debug_info_trace_ir_mapping_status copy_packet_content(
		const bt_packet *in_packet, bt_packet *out_packet,
		bt_logging_level log_level, bt_self_component *self_comp);
enum debug_info_trace_ir_mapping_status copy_event_content(
		const bt_event *in_event, bt_event *out_event,
		bt_logging_level log_level, bt_self_component *self_comp);
enum debug_info_trace_ir_mapping_status copy_field_content(
		const bt_field *in_field, bt_field *out_field,
		bt_logging_level log_level, bt_self_component *self_comp);

#endif /* BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_DATA_COPY_H */
