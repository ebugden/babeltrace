/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2013, 2014 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 */

#define BT_LOG_TAG "LIB/EVENT-CLASS"
#include "lib/logging.h"

#include "lib/assert-cond.h"
#include <babeltrace2/trace-ir/field-class.h>
#include <babeltrace2/trace-ir/event-class.h>
#include <babeltrace2/trace-ir/stream-class.h>
#include "compat/compiler.h"
#include "compat/endian.h"
#include <babeltrace2/types.h>
#include "lib/value.h"
#include "common/assert.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "event-class.h"
#include "event.h"
#include "field-class.h"
#include "resolve-field-path.h"
#include "stream-class.h"
#include "lib/func-status.h"

#define BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(_ec)				\
	BT_ASSERT_PRE_DEV_HOT("event-class",				\
		((const struct bt_event_class *) (_ec)),		\
		"Event class", ": %!+E", (_ec))

static
void destroy_event_class(struct bt_object *obj)
{
	struct bt_event_class *event_class = (void *) obj;

	BT_LIB_LOGD("Destroying event class: %!+E", event_class);
	BT_OBJECT_PUT_REF_AND_RESET(event_class->user_attributes);

	g_free(event_class->ns);
	g_free(event_class->name);
	g_free(event_class->uid);
	g_free(event_class->emf_uri);
	BT_LOGD_STR("Putting context field class.");
	BT_OBJECT_PUT_REF_AND_RESET(event_class->specific_context_fc);
	BT_LOGD_STR("Putting payload field class.");
	BT_OBJECT_PUT_REF_AND_RESET(event_class->payload_fc);
	bt_object_pool_finalize(&event_class->event_pool);
	g_free(obj);
}

static
void free_event(struct bt_event *event,
		struct bt_event_class *event_class __attribute__((unused)))
{
	bt_event_destroy(event);
}

static
bool event_class_id_is_unique(const struct bt_stream_class *stream_class,
		uint64_t id)
{
	uint64_t i;
	bool is_unique = true;

	for (i = 0; i < stream_class->event_classes->len; i++) {
		const struct bt_event_class *ec =
			stream_class->event_classes->pdata[i];

		if (ec->id == id) {
			is_unique = false;
			goto end;
		}
	}

end:
	return is_unique;
}

static
struct bt_event_class *create_event_class_with_id(
		struct bt_stream_class *stream_class, uint64_t id)
{
	int ret;
	struct bt_event_class *event_class;

	BT_ASSERT(stream_class);
	BT_ASSERT_PRE("event-class-id-is-unique",
		event_class_id_is_unique(stream_class, id),
		"Duplicate event class ID: %![sc-]+S, id=%" PRIu64,
		stream_class, id);
	BT_LIB_LOGD("Creating event class object: %![sc-]+S, id=%" PRIu64,
		stream_class, id);
	event_class = g_new0(struct bt_event_class, 1);
	if (!event_class) {
		BT_LIB_LOGE_APPEND_CAUSE("Failed to allocate one event class.");
		goto error;
	}

	bt_object_init_shared_with_parent(&event_class->base,
		destroy_event_class);
	event_class->user_attributes = bt_value_map_create();
	if (!event_class->user_attributes) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to create a map value object.");
		goto error;
	}

	event_class->id = id;
	bt_property_uint_init(&event_class->log_level,
			BT_PROPERTY_AVAILABILITY_NOT_AVAILABLE, 0);

	ret = bt_object_pool_initialize(&event_class->event_pool,
		(bt_object_pool_new_object_func) bt_event_new,
		(bt_object_pool_destroy_object_func) free_event,
		event_class);
	if (ret) {
		BT_LIB_LOGE_APPEND_CAUSE(
			"Failed to initialize event pool: ret=%d",
			ret);
		goto error;
	}

	bt_object_set_parent(&event_class->base, &stream_class->base);
	g_ptr_array_add(stream_class->event_classes, event_class);
	bt_stream_class_freeze(stream_class);
	BT_LIB_LOGD("Created event class object: %!+E", event_class);
	goto end;

error:
	BT_OBJECT_PUT_REF_AND_RESET(event_class);

end:
	return event_class;
}

BT_EXPORT
struct bt_event_class *bt_event_class_create(
		struct bt_stream_class *stream_class)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_SC_NON_NULL(stream_class);
	BT_ASSERT_PRE(
		"stream-class-automatically-assigns-event-class-ids",
		stream_class->assigns_automatic_event_class_id,
		"Stream class does not automatically assigns event class IDs: "
		"%![sc-]+S", stream_class);
	return create_event_class_with_id(stream_class,
		(uint64_t) stream_class->event_classes->len);
}

BT_EXPORT
struct bt_event_class *bt_event_class_create_with_id(
		struct bt_stream_class *stream_class, uint64_t id)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE(
		"stream-class-does-not-automatically-assigns-event-class-ids",
		!stream_class->assigns_automatic_event_class_id,
		"Stream class automatically assigns event class IDs: "
		"%![sc-]+S", stream_class);
	return create_event_class_with_id(stream_class, id);
}

BT_EXPORT
const char *bt_event_class_get_namespace(const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_EC_MIP_VERSION_GE(event_class, 1);
	return event_class->ns;
}

BT_EXPORT
enum bt_event_class_set_namespace_status bt_event_class_set_namespace(
		struct bt_event_class *event_class, const char *ns)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_EC_MIP_VERSION_GE(event_class, 1);
	BT_ASSERT_PRE_NAMESPACE_NON_NULL(ns);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	g_free(event_class->ns);
	event_class->ns = g_strdup(ns);
	BT_LIB_LOGD("Set event class's namespace: %!+E", event_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
const char *bt_event_class_get_name(const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->name;
}

BT_EXPORT
enum bt_event_class_set_name_status bt_event_class_set_name(
		struct bt_event_class *event_class, const char *name)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_NAME_NON_NULL(name);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	g_free(event_class->name);
	event_class->name = g_strdup(name);
	BT_LIB_LOGD("Set event class's name: %!+E", event_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
const char *bt_event_class_get_uid(const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_EC_MIP_VERSION_GE(event_class, 1);
	return event_class->uid;
}

BT_EXPORT
enum bt_event_class_set_uid_status bt_event_class_set_uid(
		struct bt_event_class *event_class, const char *uid)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_EC_MIP_VERSION_GE(event_class, 1);
	BT_ASSERT_PRE_NAME_NON_NULL(uid);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	g_free(event_class->uid);
	event_class->uid = g_strdup(uid);
	BT_LIB_LOGD("Set event class's UID: %!+E", event_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
uint64_t bt_event_class_get_id(const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->id;
}

BT_EXPORT
enum bt_property_availability bt_event_class_get_log_level(
		const struct bt_event_class *event_class,
		enum bt_event_class_log_level *log_level)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_DEV_NON_NULL("log-level-output", log_level,
		"Log level (output)");
	*log_level = (enum bt_event_class_log_level)
		event_class->log_level.value;
	return event_class->log_level.base.avail;
}

BT_EXPORT
void bt_event_class_set_log_level(
		struct bt_event_class *event_class,
		enum bt_event_class_log_level log_level)
{
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	bt_property_uint_set(&event_class->log_level,
		(uint64_t) log_level);
	BT_LIB_LOGD("Set event class's log level: %!+E", event_class);
}

BT_EXPORT
const char *bt_event_class_get_emf_uri(const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->emf_uri;
}

BT_EXPORT
enum bt_event_class_set_emf_uri_status bt_event_class_set_emf_uri(
		struct bt_event_class *event_class,
		const char *emf_uri)
{
	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_NON_NULL("emf-uri", emf_uri, "EMF URI");
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	g_free(event_class->emf_uri);
	event_class->emf_uri = g_strdup(emf_uri);
	BT_LIB_LOGD("Set event class's EMF URI: %!+E", event_class);
	return BT_FUNC_STATUS_OK;
}

BT_EXPORT
struct bt_stream_class *bt_event_class_borrow_stream_class(
		struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return bt_event_class_borrow_stream_class_inline(event_class);
}

BT_EXPORT
const struct bt_stream_class *
bt_event_class_borrow_stream_class_const(
		const struct bt_event_class *event_class)
{
	return bt_event_class_borrow_stream_class((void *) event_class);
}

BT_EXPORT
const struct bt_field_class *
bt_event_class_borrow_specific_context_field_class_const(
		const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->specific_context_fc;
}

BT_EXPORT
struct bt_field_class *
bt_event_class_borrow_specific_context_field_class(
		struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->specific_context_fc;
}

BT_EXPORT
enum bt_event_class_set_field_class_status
bt_event_class_set_specific_context_field_class(
		struct bt_event_class *event_class,
		struct bt_field_class *field_class)
{
	enum bt_event_class_set_field_class_status status;
	enum bt_resolve_field_xref_status resolve_status;
	struct bt_stream_class *stream_class;
	struct bt_resolve_field_xref_context resolve_ctx = {
		.packet_context = NULL,
		.event_common_context = NULL,
		.event_specific_context = field_class,
		.event_payload = NULL,
	};

	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_FC_NON_NULL(field_class);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	BT_ASSERT_PRE_FC_IS_STRUCT("specific-context", field_class,
		"Specific context field class");
	stream_class = bt_event_class_borrow_stream_class_inline(
		event_class);
	resolve_ctx.packet_context = stream_class->packet_context_fc;
	resolve_ctx.event_common_context =
		stream_class->event_common_context_fc;

	resolve_status = bt_resolve_field_paths(field_class, &resolve_ctx, __func__);
	if (resolve_status != BT_RESOLVE_FIELD_XREF_STATUS_OK) {
		status = (int) resolve_status;
		goto end;
	}

	bt_field_class_make_part_of_trace_class(field_class);
	bt_object_put_ref(event_class->specific_context_fc);
	event_class->specific_context_fc = field_class;
	bt_object_get_ref_no_null_check(event_class->specific_context_fc);
	bt_field_class_freeze(field_class);
	BT_LIB_LOGD("Set event class's specific context field class: %!+E",
		event_class);

	status = BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_OK;

end:
	return status;
}

BT_EXPORT
const struct bt_field_class *bt_event_class_borrow_payload_field_class_const(
		const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->payload_fc;
}

BT_EXPORT
struct bt_field_class *bt_event_class_borrow_payload_field_class(
		struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->payload_fc;
}

BT_EXPORT
enum bt_event_class_set_field_class_status
bt_event_class_set_payload_field_class(
		struct bt_event_class *event_class,
		struct bt_field_class *field_class)
{
	enum bt_event_class_set_field_class_status status;
	enum bt_resolve_field_xref_status resolve_status;
	struct bt_stream_class *stream_class;
	struct bt_resolve_field_xref_context resolve_ctx = {
		.packet_context = NULL,
		.event_common_context = NULL,
		.event_specific_context = NULL,
		.event_payload = field_class,
	};

	BT_ASSERT_PRE_NO_ERROR();
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_FC_NON_NULL(field_class);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	BT_ASSERT_PRE_FC_IS_STRUCT("payload", field_class, "Payload field class");
	stream_class = bt_event_class_borrow_stream_class_inline(
		event_class);
	resolve_ctx.packet_context = stream_class->packet_context_fc;
	resolve_ctx.event_common_context =
		stream_class->event_common_context_fc;
	resolve_ctx.event_specific_context = event_class->specific_context_fc;

	resolve_status = bt_resolve_field_paths(field_class, &resolve_ctx, __func__);
	if (resolve_status != BT_RESOLVE_FIELD_XREF_STATUS_OK) {
		status = (int) resolve_status;
		goto end;
	}

	bt_field_class_make_part_of_trace_class(field_class);
	bt_object_put_ref(event_class->payload_fc);
	event_class->payload_fc = field_class;
	bt_object_get_ref_no_null_check(event_class->payload_fc);
	bt_field_class_freeze(field_class);
	BT_LIB_LOGD("Set event class's payload field class: %!+E", event_class);

	status = BT_EVENT_CLASS_SET_FIELD_CLASS_STATUS_OK;

end:
	return status;
}

void _bt_event_class_freeze(const struct bt_event_class *event_class)
{
	/* The field classes are already frozen */
	BT_ASSERT(event_class);
	BT_LIB_LOGD("Freezing event class's user attributes: %!+v",
		event_class->user_attributes);
	bt_value_freeze(event_class->user_attributes);
	BT_LIB_LOGD("Freezing event class: %!+E", event_class);
	((struct bt_event_class *) event_class)->frozen = true;
}

BT_EXPORT
const struct bt_value *bt_event_class_borrow_user_attributes_const(
		const struct bt_event_class *event_class)
{
	BT_ASSERT_PRE_DEV_EC_NON_NULL(event_class);
	return event_class->user_attributes;
}

BT_EXPORT
struct bt_value *bt_event_class_borrow_user_attributes(
		struct bt_event_class *event_class)
{
	return (void *) bt_event_class_borrow_user_attributes_const(
		(void *) event_class);
}

BT_EXPORT
void bt_event_class_set_user_attributes(
		struct bt_event_class *event_class,
		const struct bt_value *user_attributes)
{
	BT_ASSERT_PRE_EC_NON_NULL(event_class);
	BT_ASSERT_PRE_DEV_EVENT_CLASS_HOT(event_class);
	BT_ASSERT_PRE_USER_ATTRS_NON_NULL(user_attributes);
	BT_ASSERT_PRE_USER_ATTRS_IS_MAP(user_attributes);
	bt_object_put_ref_no_null_check(event_class->user_attributes);
	event_class->user_attributes = (void *) user_attributes;
	bt_object_get_ref_no_null_check(event_class->user_attributes);
}

BT_EXPORT
void bt_event_class_get_ref(const struct bt_event_class *event_class)
{
	bt_object_get_ref(event_class);
}

BT_EXPORT
void bt_event_class_put_ref(const struct bt_event_class *event_class)
{
	bt_object_put_ref(event_class);
}
