/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017 Philippe Proulx <pproulx@efficios.com>
 */

#define BT_COMP_LOG_SELF_COMP self_comp
#define BT_LOG_OUTPUT_LEVEL log_level
#define BT_LOG_TAG "PLUGIN/SINK.UTILS.DUMMY"
#include "logging/comp-logging.h"

#include <babeltrace2/babeltrace.h>
#include "common/macros.h"
#include "common/assert.h"
#include "dummy.h"
#include "plugins/common/param-validation/param-validation.h"

static
const char * const in_port_name = "in";

bt_component_class_get_supported_mip_versions_method_status
dummy_supported_mip_versions(
		bt_self_component_class_sink *self_component_class __attribute__((unused)),
		const bt_value *params __attribute__((unused)),
		void *initialize_method_data __attribute__((unused)),
		bt_logging_level logging_level __attribute__((unused)),
		bt_integer_range_set_unsigned *supported_versions) {
	return (int) bt_integer_range_set_unsigned_add_range(supported_versions, 0, 1);
}

static
void destroy_private_dummy_data(struct dummy *dummy)
{
	bt_message_iterator_put_ref(dummy->msg_iter);
	g_free(dummy);

}

void dummy_finalize(bt_self_component_sink *comp)
{
	struct dummy *dummy;

	BT_ASSERT(comp);
	dummy = bt_self_component_get_data(
			bt_self_component_sink_as_self_component(comp));
	BT_ASSERT(dummy);
	destroy_private_dummy_data(dummy);
}

static
struct bt_param_validation_map_value_entry_descr dummy_params[] = {
	BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_END
};

bt_component_class_initialize_method_status dummy_init(
		bt_self_component_sink *self_comp_sink,
		bt_self_component_sink_configuration *config __attribute__((unused)),
		const bt_value *params,
		void *init_method_data __attribute__((unused)))
{
	bt_self_component *self_comp =
		bt_self_component_sink_as_self_component(self_comp_sink);
	const bt_component *comp = bt_self_component_as_component(self_comp);
	bt_logging_level log_level = bt_component_get_logging_level(comp);
	bt_component_class_initialize_method_status status;
	bt_self_component_add_port_status add_port_status;
	struct dummy *dummy = g_new0(struct dummy, 1);
	enum bt_param_validation_status validation_status;
	gchar *validate_error = NULL;

	if (!dummy) {
		status = BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_MEMORY_ERROR;
		goto end;
	}

	validation_status = bt_param_validation_validate(params,
		dummy_params, &validate_error);
	if (validation_status == BT_PARAM_VALIDATION_STATUS_MEMORY_ERROR) {
		status = BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_MEMORY_ERROR;
		goto error;
	} else if (validation_status == BT_PARAM_VALIDATION_STATUS_VALIDATION_ERROR) {
		status = BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_ERROR;
		BT_COMP_LOGE_APPEND_CAUSE(self_comp, "%s", validate_error);
		goto error;
	}

	add_port_status = bt_self_component_sink_add_input_port(self_comp_sink,
		"in", NULL, NULL);
	if (add_port_status != BT_SELF_COMPONENT_ADD_PORT_STATUS_OK) {
		status = (int) add_port_status;
		goto error;
	}

	bt_self_component_set_data(self_comp, dummy);

	status = BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
	goto end;

error:
	destroy_private_dummy_data(dummy);

end:
	g_free(validate_error);

	return status;
}

bt_component_class_sink_graph_is_configured_method_status dummy_graph_is_configured(
		bt_self_component_sink *comp)
{
	bt_component_class_sink_graph_is_configured_method_status status;
	bt_message_iterator_create_from_sink_component_status
		msg_iter_status;
	struct dummy *dummy;
	bt_message_iterator *iterator;

	dummy = bt_self_component_get_data(
		bt_self_component_sink_as_self_component(comp));
	BT_ASSERT(dummy);
	msg_iter_status = bt_message_iterator_create_from_sink_component(
		comp, bt_self_component_sink_borrow_input_port_by_name(comp,
			in_port_name), &iterator);
	if (msg_iter_status != BT_MESSAGE_ITERATOR_CREATE_FROM_SINK_COMPONENT_STATUS_OK) {
		status = (int) msg_iter_status;
		goto end;
	}

	BT_MESSAGE_ITERATOR_MOVE_REF(
		dummy->msg_iter, iterator);

	status = BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;

end:
	return status;
}

bt_component_class_sink_consume_method_status dummy_consume(
		bt_self_component_sink *component)
{
	bt_message_array_const msgs;
	uint64_t count;
	struct dummy *dummy;
	bt_message_iterator_next_status next_status;
	uint64_t i;
	bt_self_component *self_comp =
		bt_self_component_sink_as_self_component(component);

	dummy = bt_self_component_get_data(self_comp);
	BT_ASSERT_DBG(dummy);
	BT_ASSERT_DBG(dummy->msg_iter);

	/* Consume one message  */
	next_status = bt_message_iterator_next(
		dummy->msg_iter, &msgs, &count);
	switch (next_status) {
	case BT_MESSAGE_ITERATOR_NEXT_STATUS_OK:
		for (i = 0; i < count; i++) {
			bt_message_put_ref(msgs[i]);
		}

		break;
	case BT_MESSAGE_ITERATOR_NEXT_STATUS_ERROR:
	case BT_MESSAGE_ITERATOR_NEXT_STATUS_MEMORY_ERROR:
		BT_CURRENT_THREAD_ERROR_APPEND_CAUSE_FROM_COMPONENT(
			self_comp,
			"Failed to get messages from upstream component");
		break;
	default:
		break;
	}

	return (int) next_status;
}
