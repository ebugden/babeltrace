/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019 Francis Deslauriers francis.deslauriers@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_MAPPING_H
#define BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_MAPPING_H

#include <glib.h>

#include "common/assert.h"
#include "common/macros.h"
#include <babeltrace2/babeltrace.h>

#include "debug-info.hpp"

enum debug_info_trace_ir_mapping_status {
	DEBUG_INFO_TRACE_IR_MAPPING_STATUS_OK = 0,
	DEBUG_INFO_TRACE_IR_MAPPING_STATUS_MEMORY_ERROR = -12,
};

/* Used to resolve field paths for dynamic arrays and variant field classes. */
struct field_class_resolving_context {
	/* Weak reference. Owned by input stream class. */
	const bt_field_class *packet_context;
	/* Weak reference. Owned by input stream class. */
	const bt_field_class *event_common_context;
	/* Weak reference. Owned by input event class. */
	const bt_field_class *event_specific_context;
	/* Weak reference. Owned by input event class. */
	const bt_field_class *event_payload;
};

struct trace_ir_metadata_maps {
	bt_logging_level log_level;
	bt_self_component *self_comp;
	const bt_trace_class *input_trace_class;
	bt_trace_class *output_trace_class;

	/*
	 * Map between input stream class and its corresponding output stream
	 * class.
	 * input stream class: weak reference. Owned by an upstream
	 * component.
	 * output stream class: owned by this structure.
	 */
	GHashTable *stream_class_map;

	/*
	 * Map between input event class and its corresponding output event
	 * class.
	 * input event class: weak reference. Owned by an upstream component.
	 * output event class: owned by this structure.
	 */
	GHashTable *event_class_map;

	/*
	 * Map between input field class and its corresponding output field
	 * class.
	 * input field class: weak reference. Owned by an upstream component.
	 * output field class: owned by this structure.
	 */
	GHashTable *field_class_map;

	/*
	 * Map between input clock class and its corresponding output clock
	 * class.
	 * input clock class: weak reference. Owned by an upstream component.
	 * output clock class: owned by this structure.
	 */
	GHashTable *clock_class_map;

	struct field_class_resolving_context *fc_resolving_ctx;

	bt_listener_id destruction_listener_id;
};

struct trace_ir_data_maps {
	bt_logging_level log_level;
	bt_self_component *self_comp;
	const bt_trace *input_trace;
	bt_trace *output_trace;

	/*
	 * Map between input stream its corresponding output stream.
	 * input stream: weak reference. Owned by an upstream component.
	 * output stream: owned by this structure.
	 */
	GHashTable *stream_map;

	/*
	 * Map between input packet its corresponding output packet.
	 * input packet: weak reference. Owned by an upstream packet component.
	 * output packet: owned by this structure.
	 */
	GHashTable *packet_map;

	bt_listener_id destruction_listener_id;
};

struct trace_ir_maps {
	bt_logging_level log_level;

	/*
	 * input trace -> trace_ir_data_maps.
	 * input trace: weak reference. Owned by an upstream component.
	 * trace_ir_data_maps: Owned by this structure.
	 */
	GHashTable *data_maps;

	/*
	 * input trace class -> trace_ir_metadata_maps.
	 * input trace class: weak reference. Owned by an upstream component.
	 * trace_ir_metadata_maps: Owned by this structure.
	 */
	GHashTable *metadata_maps;

	char *debug_info_field_class_name;

	bt_self_component *self_comp;
};

struct trace_ir_maps *trace_ir_maps_create(bt_self_component *self_comp,
		const char *debug_info_field_name, bt_logging_level log_level);

void trace_ir_maps_clear(struct trace_ir_maps *maps);

void trace_ir_maps_destroy(struct trace_ir_maps *maps);

struct trace_ir_data_maps *trace_ir_data_maps_create(
		struct trace_ir_maps *ir_maps,
		const bt_trace *in_trace);

void trace_ir_data_maps_destroy(struct trace_ir_data_maps *d_maps);

struct trace_ir_metadata_maps *trace_ir_metadata_maps_create(
		struct trace_ir_maps *ir_maps,
		const bt_trace_class *in_trace_class);

void trace_ir_metadata_maps_destroy(struct trace_ir_metadata_maps *md_maps);

bt_stream_class *trace_ir_mapping_create_new_mapped_stream_class(
		struct trace_ir_maps *ir_maps,
		const bt_stream_class *in_stream_class);

bt_stream_class *trace_ir_mapping_borrow_mapped_stream_class(
		struct trace_ir_maps *ir_maps,
		const bt_stream_class *in_stream_class);

bt_stream *trace_ir_mapping_create_new_mapped_stream(
		struct trace_ir_maps *ir_maps,
		const bt_stream *in_stream);

bt_stream *trace_ir_mapping_borrow_mapped_stream(
		struct trace_ir_maps *ir_maps,
		const bt_stream *in_stream);

void trace_ir_mapping_remove_mapped_stream(
		struct trace_ir_maps *ir_maps,
		const bt_stream *in_stream);

bt_event_class *trace_ir_mapping_create_new_mapped_event_class(
		struct trace_ir_maps *ir_maps,
		const bt_event_class *in_event_class);

bt_event_class *trace_ir_mapping_borrow_mapped_event_class(
		struct trace_ir_maps *ir_maps,
		const bt_event_class *in_event_class);

bt_packet *trace_ir_mapping_create_new_mapped_packet(
		struct trace_ir_maps *ir_maps,
		const bt_packet *in_packet);

bt_packet *trace_ir_mapping_borrow_mapped_packet(
		struct trace_ir_maps *ir_maps,
		const bt_packet *in_packet);

void trace_ir_mapping_remove_mapped_packet(
		struct trace_ir_maps *ir_maps,
		const bt_packet *in_packet);

static inline
struct trace_ir_data_maps *borrow_data_maps_from_input_trace(
		struct trace_ir_maps *ir_maps, const bt_trace *in_trace)
{
	BT_ASSERT(ir_maps);
	BT_ASSERT(in_trace);

	struct trace_ir_data_maps *d_maps =
		static_cast<trace_ir_data_maps *>(g_hash_table_lookup(ir_maps->data_maps, (gpointer) in_trace));
	if (!d_maps) {
		d_maps = trace_ir_data_maps_create(ir_maps, in_trace);
		g_hash_table_insert(ir_maps->data_maps, (gpointer) in_trace, d_maps);
	}

	return d_maps;
}

static inline
struct trace_ir_data_maps *borrow_data_maps_from_input_stream(
		struct trace_ir_maps *ir_maps, const bt_stream *in_stream)
{
	BT_ASSERT(ir_maps);
	BT_ASSERT(in_stream);

	return borrow_data_maps_from_input_trace(ir_maps,
			bt_stream_borrow_trace_const(in_stream));
}

static inline
struct trace_ir_data_maps *borrow_data_maps_from_input_packet(
		struct trace_ir_maps *ir_maps, const bt_packet *in_packet)
{
	BT_ASSERT(ir_maps);
	BT_ASSERT(in_packet);

	return borrow_data_maps_from_input_stream(ir_maps,
			bt_packet_borrow_stream_const(in_packet));
}

static inline
struct trace_ir_metadata_maps *borrow_metadata_maps_from_input_trace_class(
		struct trace_ir_maps *ir_maps,
		const bt_trace_class *in_trace_class)
{
	BT_ASSERT(ir_maps);
	BT_ASSERT(in_trace_class);

	struct trace_ir_metadata_maps *md_maps =
		static_cast<trace_ir_metadata_maps *>(g_hash_table_lookup(ir_maps->metadata_maps,
				(gpointer) in_trace_class));
	if (!md_maps) {
		md_maps = trace_ir_metadata_maps_create(ir_maps, in_trace_class);
		g_hash_table_insert(ir_maps->metadata_maps,
				(gpointer) in_trace_class, md_maps);
	}

	return md_maps;
}

static inline
struct trace_ir_metadata_maps *borrow_metadata_maps_from_input_stream_class(
		struct trace_ir_maps *ir_maps,
		const bt_stream_class *in_stream_class) {

	BT_ASSERT(in_stream_class);

	return borrow_metadata_maps_from_input_trace_class(ir_maps,
			bt_stream_class_borrow_trace_class_const(in_stream_class));
}

static inline
struct trace_ir_metadata_maps *borrow_metadata_maps_from_input_event_class(
		struct trace_ir_maps *ir_maps,
		const bt_event_class *in_event_class) {

	BT_ASSERT(in_event_class);

	return borrow_metadata_maps_from_input_stream_class(ir_maps,
			bt_event_class_borrow_stream_class_const(in_event_class));
}

#endif /* BABELTRACE_PLUGINS_LTTNG_UTILS_DEBUG_INFO_TRACE_IR_MAPPING_H */
