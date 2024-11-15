/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2013, 2014 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 */

#define BT_LOG_TAG "LIB/STREAM-CLASS"
#include "lib/logging.h"

#include "lib/assert-cond.h"
#include <babeltrace2/trace-ir/trace.h>
#include "compat/compiler.h"
#include "compat/endian.h"
#include "common/assert.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include "clock-class.h"
#include "event-class.h"
#include "field-class.h"
#include "field-wrapper.h"
#include "resolve-field-path.h"
#include "stream-class.h"
#include "lib/value.h"
#include "lib/func-status.h"
#include "trace-class.h"

#define BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(_sc)				\
	BT_ASSERT_PRE_DEV_HOT("stream-class", (_sc), "Stream class",	\
		": %!+S", (_sc))

static
void destroy_stream_class(struct bt_object *obj)
{
	struct bt_stream_class *stream_class = (void *) obj;

	BT_LIB_LOGD("Destroying stream class: %!+S", stream_class);
	BT_LOGD_STR("Putting default clock class.");
	BT_OBJECT_PUT_REF_AND_RESET(stream_class->user_attributes);
	BT_OBJECT_PUT_REF_AND_RESET(stream_class->default_clock_class);

	if (stream_class->event_classes) {
		BT_LOGD_STR("Destroying event classes.");
		g_ptr_array_free(stream_class->event_classes, TRUE);
		stream_class->event_classes = NULL;
	}

	g_free(stream_class->ns);
	g_free(stream_class->name);
	g_free(stream_class->uid);
	BT_LOGD_STR("Putting packet context field class.");
	BT_OBJECT_PUT_REF_AND_RESET(stream_class->packet_context_fc);
	BT_LOGD_STR("Putting event common context field class.");
	BT_OBJECT_PUT_REF_AND_RESET(stream_class->event_common_context_fc);
	bt_object_pool_finalize(&stream_class->packet_context_field_pool);
	g_free(stream_class);
}

static
void free_field_wrapper(struct bt_field_wrapper *field_wrapper,
		struct bt_stream_class *stream_class __attribute__((unused)))
{
	bt_field_wrapper_destroy((void *) field_wrapper);
}

static
bool stream_class_id_is_unique(const struct bt_trace_class *tc, uint64_t id)
{
	uint64_t i;
	bool is_unique = true;

	for (i = 0; i < tc->stream_classes->len; i++) {
		const struct bt_stream_class *sc =
			tc->stream_classes->pdata[i];

		if (sc->id == id) {
			is_unique = false;
			goto end;
		}
	}

end:
	return is_unique;
}

static
struct bt_stream_class *create_stream_class_with_id(
		struct bt_trace_class *tc, uint64_t id)
{
	struct bt_stream_class *stream_class = NULL;
	int ret;

	BT_ASSERT(tc);
	BT_ASSERT_PRE("stream-class-id-is-unique",
		stream_class_id_is_unique(tc, id),
		"Duplicate stream class ID: %![tc-]+T, id=%" PRIu64, tc, id);
	BT_LIB_LOGD("Creating stream class object: %![tc-]+T, id=%" PRIu64,
		tc, id);
	stream_class = g_new0(struct bt_stream_class, 1);
	if (!stream_class) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to allocate one stream class.");
		goto error;
	}

	bt_object_init_shared_with_parent(&stream_class->base,
		destroy_stream_class);
	stream_class->user_attributes = bt_value_map_create();
	if (!stream_class->user_attributes) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to create a map value object.");
		goto error;
	}

	stream_class->id = id;
	stream_class->assigns_automatic_event_class_id = true;
	stream_class->assigns_automatic_stream_id = true;
	stream_class->event_classes = g_ptr_array_new_with_free_func(
		(GDestroyNotify) bt_object_try_spec_release);
	if (!stream_class->event_classes) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate a GPtrArray.");
		goto error;
	}

	ret = bt_object_pool_initialize(&stream_class->packet_context_field_pool,
		(bt_object_pool_new_object_func) bt_field_wrapper_new,
		(bt_object_pool_destroy_object_func) free_field_wrapper,
		stream_class);
	if (ret) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to initialize packet context field pool: ret=%d",
			ret);
		goto error;
	}

	bt_object_set_parent(&stream_class->base, &tc->base);
	g_ptr_array_add(tc->stream_classes, stream_class);
	bt_trace_class_freeze(tc);
	BT_LIB_LOGD("Created stream class object: %!+S", stream_class);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(stream_class);

end:
	return stream_class;
}

BT_EXPORT
struct bt_stream_class *bt_stream_class_create(struct bt_trace_class *tc)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_TC_NON_NULL(tc);
	BT_ASSERT_PRE("trace-class-automatically-assigns-stream-class-ids",
		tc->assigns_automatic_stream_class_id,
		"Trace class does not automatically assigns stream class IDs: "
		"%![sc-]+T", tc);
	return create_stream_class_with_id(tc,
		(uint64_t) tc->stream_classes->len);
}

BT_EXPORT
struct bt_stream_class *bt_stream_class_create_with_id(
		struct bt_trace_class *tc, uint64_t id)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_TC_NON_NULL(tc);
	BT_ASSERT_PRE(
		"trace-class-does-not-automatically-assigns-stream-class-ids",
		!tc->assigns_automatic_stream_class_id,
		"Trace class automatically assigns stream class IDs: "
		"%![sc-]+T", tc);
	return create_stream_class_with_id(tc, id);
}

BT_EXPORT
struct bt_trace_class *bt_stream_class_borrow_trace_class(
		struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return bt_stream_class_borrow_trace_class_inline(stream_class);
}

BT_EXPORT
const struct bt_trace_class *bt_stream_class_borrow_trace_class_const(
		const struct bt_stream_class *stream_class)
{
	return bt_stream_class_borrow_trace_class((void *) stream_class);
}

BT_EXPORT
const char *bt_stream_class_get_namespace(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_SC_MIP_VERSION_GE(stream_class, 1);
	return stream_class->ns;
}

BT_EXPORT
enum bt_stream_class_set_namespace_status bt_stream_class_set_namespace(
		struct bt_stream_class *stream_class,
		const char *ns)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_SC_MIP_VERSION_GE(stream_class, 1);
	BT_ASSERT_PRE_NAMESPACE_NON_NULL(ns);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	g_free(stream_class->ns);
	stream_class->ns = g_strdup(ns);
	BT_LIB_LOGD("Set stream class's namespace: %!+S", stream_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
const char *bt_stream_class_get_name(const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->name;
}

BT_EXPORT
enum bt_stream_class_set_name_status bt_stream_class_set_name(
		struct bt_stream_class *stream_class,
		const char *name)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_NAME_NON_NULL(name);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	g_free(stream_class->name);
	stream_class->name = g_strdup(name);
	BT_LIB_LOGD("Set stream class's name: %!+S", stream_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
const char *bt_stream_class_get_uid(const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_SC_MIP_VERSION_GE(stream_class, 1);
	return stream_class->uid;
}

BT_EXPORT
enum bt_stream_class_set_uid_status bt_stream_class_set_uid(
		struct bt_stream_class *stream_class,
		const char *uid)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_NAME_NON_NULL(uid);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE_SC_MIP_VERSION_GE(stream_class, 1);
	g_free(stream_class->uid);
	stream_class->uid = g_strdup(uid);
	BT_LIB_LOGD("Set stream class's UID: %!+S", stream_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
uint64_t bt_stream_class_get_id(const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->id;
}

BT_EXPORT
uint64_t bt_stream_class_get_event_class_count(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (uint64_t) stream_class->event_classes->len;
}

BT_EXPORT
struct bt_event_class *bt_stream_class_borrow_event_class_by_index(
		struct bt_stream_class *stream_class, uint64_t index)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_VALID_INDEX(index, stream_class->event_classes->len);
	return g_ptr_array_index(stream_class->event_classes, index);
}

BT_EXPORT
const struct bt_event_class *
bt_stream_class_borrow_event_class_by_index_const(
		const struct bt_stream_class *stream_class, uint64_t index)
{
	return bt_stream_class_borrow_event_class_by_index(
		(void *) stream_class, index);
}

BT_EXPORT
struct bt_event_class *bt_stream_class_borrow_event_class_by_id(
		struct bt_stream_class *stream_class, uint64_t id)
{
	struct bt_event_class *event_class = NULL;
	uint64_t i;

	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);

	for (i = 0; i < stream_class->event_classes->len; i++) {
		struct bt_event_class *event_class_candidate =
			g_ptr_array_index(stream_class->event_classes, i);

		if (event_class_candidate->id == id) {
			event_class = event_class_candidate;
			goto end;
		}
	}

end:
	return event_class;
}

BT_EXPORT
const struct bt_event_class *
bt_stream_class_borrow_event_class_by_id_const(
		const struct bt_stream_class *stream_class, uint64_t id)
{
	return bt_stream_class_borrow_event_class_by_id(
		(void *) stream_class, id);
}

BT_EXPORT
const struct bt_field_class *
bt_stream_class_borrow_packet_context_field_class_const(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->packet_context_fc;
}

BT_EXPORT
struct bt_field_class *
bt_stream_class_borrow_packet_context_field_class(
		struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->packet_context_fc;
}

BT_EXPORT
enum bt_stream_class_set_field_class_status
bt_stream_class_set_packet_context_field_class(
		struct bt_stream_class *stream_class,
		struct bt_field_class *field_class)
{
	enum bt_stream_class_set_field_class_status status;
	enum bt_resolve_field_xref_status resolve_status;
	struct bt_resolve_field_xref_context resolve_ctx = {
		.packet_context = field_class,
		.event_common_context = NULL,
		.event_specific_context = NULL,
		.event_payload = NULL,
	};

	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE("supports-packets",
		stream_class->supports_packets,
		"Stream class does not support packets: %![sc-]+S",
		stream_class);
	BT_ASSERT_PRE_FC_NON_NULL(field_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE_FC_IS_STRUCT("field-class", field_class,
		"Packet context field class");
	resolve_status = bt_resolve_field_paths(field_class, &resolve_ctx, __func__);
	if (resolve_status != BT_RESOLVE_FIELD_XREF_STATUS_OK) {
		status = (int) resolve_status;
		goto end;
	}

	bt_field_class_make_part_of_trace_class(field_class);
	bt_object_put_ref(stream_class->packet_context_fc);
	stream_class->packet_context_fc = field_class;
	bt_object_get_ref_no_null_check(stream_class->packet_context_fc);
	bt_field_class_freeze(field_class);
	BT_LIB_LOGD("Set stream class's packet context field class: %!+S",
		stream_class);

	status = BT_STREAM_CLASS_SET_FIELD_CLASS_STATUS_OK;

end:
	return status;
}

BT_EXPORT
const struct bt_field_class *
bt_stream_class_borrow_event_common_context_field_class_const(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->event_common_context_fc;
}

BT_EXPORT
struct bt_field_class *
bt_stream_class_borrow_event_common_context_field_class(
		struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->event_common_context_fc;
}

BT_EXPORT
enum bt_stream_class_set_field_class_status
bt_stream_class_set_event_common_context_field_class(
		struct bt_stream_class *stream_class,
		struct bt_field_class *field_class)
{
	enum bt_stream_class_set_field_class_status status;
	enum bt_resolve_field_xref_status resolve_status;
	struct bt_resolve_field_xref_context resolve_ctx = {
		.packet_context = NULL,
		.event_common_context = field_class,
		.event_specific_context = NULL,
		.event_payload = NULL,
	};

	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_FC_NON_NULL(field_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE_FC_IS_STRUCT("field-class", field_class,
		"Event common context field class");
	resolve_ctx.packet_context = stream_class->packet_context_fc;
	resolve_status = bt_resolve_field_paths(field_class, &resolve_ctx, __func__);
	if (resolve_status != BT_RESOLVE_FIELD_XREF_STATUS_OK) {
		status = (int) resolve_status;
		goto end;
	}

	bt_field_class_make_part_of_trace_class(field_class);
	bt_object_put_ref(stream_class->event_common_context_fc);
	stream_class->event_common_context_fc = field_class;
	bt_object_get_ref_no_null_check(stream_class->event_common_context_fc);
	bt_field_class_freeze(field_class);
	BT_LIB_LOGD("Set stream class's event common context field class: %!+S",
		stream_class);

	status = BT_STREAM_CLASS_SET_FIELD_CLASS_STATUS_OK;

end:
	return status;
}

void _bt_stream_class_freeze(const struct bt_stream_class *stream_class)
{
	/* The field classes and default clock class are already frozen */
	BT_ASSERT(stream_class);
	BT_LIB_LOGD("Freezing stream class's user attributes: %!+v",
		stream_class->user_attributes);
	bt_value_freeze(stream_class->user_attributes);
	BT_LIB_LOGD("Freezing stream class: %!+S", stream_class);
	((struct bt_stream_class *) stream_class)->frozen = true;
}

BT_EXPORT
enum bt_stream_class_set_default_clock_class_status
bt_stream_class_set_default_clock_class(
		struct bt_stream_class *stream_class,
		struct bt_clock_class *clock_class)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_CLK_CLS_NON_NULL(clock_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	bt_object_put_ref(stream_class->default_clock_class);
	stream_class->default_clock_class = clock_class;
	bt_object_get_ref_no_null_check(stream_class->default_clock_class);
	bt_clock_class_freeze(clock_class);
	BT_LIB_LOGD("Set stream class's default clock class: %!+S",
		stream_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
struct bt_clock_class *bt_stream_class_borrow_default_clock_class(
		struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->default_clock_class;
}

BT_EXPORT
const struct bt_clock_class *bt_stream_class_borrow_default_clock_class_const(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->default_clock_class;
}

BT_EXPORT
bt_bool bt_stream_class_assigns_automatic_event_class_id(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->assigns_automatic_event_class_id;
}

BT_EXPORT
void bt_stream_class_set_assigns_automatic_event_class_id(
		struct bt_stream_class *stream_class,
		bt_bool value)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	stream_class->assigns_automatic_event_class_id = (bool) value;
	BT_LIB_LOGD("Set stream class's automatic event class ID "
		"assignment property: %!+S", stream_class);
}

BT_EXPORT
bt_bool bt_stream_class_assigns_automatic_stream_id(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->assigns_automatic_stream_id;
}

BT_EXPORT
void bt_stream_class_set_supports_discarded_events(
		struct bt_stream_class *stream_class,
		bt_bool supports_discarded_events,
		bt_bool with_default_clock_snapshots)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE("supports-discarded-events-for-default-clock-snapshots",
		supports_discarded_events ||
			!with_default_clock_snapshots,
		"Discarded events cannot have default clock snapshots when "
		"not supported: %!+S", stream_class);
	BT_ASSERT_PRE("has-default-clock-class-for-default-clock-snapshots",
		!with_default_clock_snapshots ||
			stream_class->default_clock_class,
		"Stream class has no default clock class: %!+S", stream_class);
	stream_class->supports_discarded_events =
		(bool) supports_discarded_events;
	stream_class->discarded_events_have_default_clock_snapshots =
		(bool) with_default_clock_snapshots;
	BT_LIB_LOGD("Set stream class's discarded events support property: "
		"%!+S", stream_class);
}

BT_EXPORT
bt_bool bt_stream_class_supports_discarded_events(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->supports_discarded_events;
}

BT_EXPORT
bt_bool bt_stream_class_discarded_events_have_default_clock_snapshots(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->discarded_events_have_default_clock_snapshots;
}

BT_EXPORT
void bt_stream_class_set_supports_discarded_packets(
		struct bt_stream_class *stream_class,
		bt_bool supports_discarded_packets,
		bt_bool with_default_clock_snapshots)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE("supports-packets-for-discarded-packets-support",
		!supports_discarded_packets ||
			stream_class->supports_packets,
		"Stream class does not support packets: %!+S",
		stream_class);
	BT_ASSERT_PRE("supports-discarded-packets-for-default-clock-snapshots",
		supports_discarded_packets ||
			!with_default_clock_snapshots,
		"Discarded packets cannot have default clock snapshots when "
		"not supported: %!+S", stream_class);
	BT_ASSERT_PRE("has-default-clock-class-for-default-clock-snapshots",
		!with_default_clock_snapshots ||
			stream_class->default_clock_class,
		"Stream class has no default clock class: %!+S", stream_class);
	stream_class->supports_discarded_packets =
		(bool) supports_discarded_packets;
	stream_class->discarded_packets_have_default_clock_snapshots =
		(bool) with_default_clock_snapshots;
	BT_LIB_LOGD("Set stream class's discarded packets support property: "
		"%!+S", stream_class);
}

BT_EXPORT
bt_bool bt_stream_class_supports_discarded_packets(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->supports_discarded_packets;
}

BT_EXPORT
bt_bool bt_stream_class_discarded_packets_have_default_clock_snapshots(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->discarded_packets_have_default_clock_snapshots;
}

BT_EXPORT
void bt_stream_class_set_supports_packets(
		struct bt_stream_class *stream_class,
		bt_bool supports_packets,
		bt_bool with_beginning_default_clock_snapshot,
		bt_bool with_end_default_clock_snapshot)
{
	bt_bool with_default_clock_snapshot =
		with_beginning_default_clock_snapshot ||
		with_end_default_clock_snapshot;
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	BT_ASSERT_PRE("supports-packets-for-default-clock-snapshot",
		supports_packets ||
			!with_default_clock_snapshot,
		"Packets cannot have default clock snapshots when "
		"not supported: %!+S", stream_class);
	BT_ASSERT_PRE("has-default-clock-class-for-default-clock-snapshot",
		!with_default_clock_snapshot ||
			stream_class->default_clock_class,
		"Stream class has no default clock class: %!+S", stream_class);
	BT_ASSERT_PRE("supports-packets-for-packet-context-field-class",
		supports_packets || !stream_class->packet_context_fc,
		"Stream class already has a packet context field class: %!+S",
		stream_class);
	BT_ASSERT_PRE("supports-packets-for-discarded-packets-support",
		supports_packets || !stream_class->supports_discarded_packets,
		"Stream class already supports discarded packets: %!+S",
		stream_class);
	stream_class->supports_packets = (bool) supports_packets;
	stream_class->packets_have_beginning_default_clock_snapshot =
		(bool) with_beginning_default_clock_snapshot;
	stream_class->packets_have_end_default_clock_snapshot =
		(bool) with_end_default_clock_snapshot;
	BT_LIB_LOGD("Set stream class's packets support property: %!+S",
		stream_class);
}

BT_EXPORT
bt_bool bt_stream_class_supports_packets(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->supports_packets;
}

BT_EXPORT
bt_bool bt_stream_class_packets_have_beginning_default_clock_snapshot(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->packets_have_beginning_default_clock_snapshot;
}

BT_EXPORT
bt_bool bt_stream_class_packets_have_end_default_clock_snapshot(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return (bt_bool) stream_class->packets_have_end_default_clock_snapshot;
}

BT_EXPORT
void bt_stream_class_set_assigns_automatic_stream_id(
		struct bt_stream_class *stream_class,
		bt_bool value)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	stream_class->assigns_automatic_stream_id = (bool) value;
	BT_LIB_LOGD("Set stream class's automatic stream ID "
		"assignment property: %!+S", stream_class);
}

BT_EXPORT
const struct bt_value *bt_stream_class_borrow_user_attributes_const(
		const struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_DEV_SC_NON_NULL(stream_class);
	return stream_class->user_attributes;
}

BT_EXPORT
struct bt_value *bt_stream_class_borrow_user_attributes(
		struct bt_stream_class *stream_class)
{
	return (void *) bt_stream_class_borrow_user_attributes_const(
		(void *) stream_class);
}

BT_EXPORT
void bt_stream_class_set_user_attributes(
		struct bt_stream_class *stream_class,
		const struct bt_value *user_attributes)
{
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE_USER_ATTRS_NON_NULL(user_attributes);
	BT_ASSERT_PRE_USER_ATTRS_IS_MAP(user_attributes);
	BT_ASSERT_PRE_DEV_STREAM_CLASS_HOT(stream_class);
	bt_object_put_ref_no_null_check(stream_class->user_attributes);
	stream_class->user_attributes = (void *) user_attributes;
	bt_object_get_ref_no_null_check(stream_class->user_attributes);
}

BT_EXPORT
void bt_stream_class_get_ref(const struct bt_stream_class *stream_class)
{
	bt_object_get_ref(stream_class);
}

BT_EXPORT
void bt_stream_class_put_ref(const struct bt_stream_class *stream_class)
{
	bt_object_put_ref(stream_class);
}
