#ifndef BABELTRACE_GRAPH_COMPONENT_CLASS_SOURCE_H
#define BABELTRACE_GRAPH_COMPONENT_CLASS_SOURCE_H

/*
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdint.h>

/* For enum bt_self_component_status */
#include <babeltrace/graph/self-component.h>

/* For enum bt_self_notification_iterator_status */
#include <babeltrace/graph/self-notification-iterator.h>

/* For enum bt_query_status */
#include <babeltrace/graph/component-class.h>

/*
 * For bt_component_class, bt_component_class_source, bt_port_input,
 * bt_query_executor, bt_self_component_class_source,
 * bt_self_component_source, bt_self_component_port_output, bt_value,
 * bt_notification_array_const
 */
#include <babeltrace/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bt_self_component_status
(*bt_component_class_source_init_method)(
		bt_self_component_source *self_component,
		const bt_value *params, void *init_method_data);

typedef void (*bt_component_class_source_finalize_method)(
		bt_self_component_source *self_component);

typedef enum bt_self_notification_iterator_status
(*bt_component_class_source_notification_iterator_init_method)(
		bt_self_notification_iterator *notification_iterator,
		bt_self_component_source *self_component,
		bt_self_component_port_output *port);

typedef void
(*bt_component_class_source_notification_iterator_finalize_method)(
		bt_self_notification_iterator *notification_iterator);

typedef enum bt_self_notification_iterator_status
(*bt_component_class_source_notification_iterator_next_method)(
		bt_self_notification_iterator *notification_iterator,
		bt_notification_array_const notifs, uint64_t capacity,
		uint64_t *count);

typedef enum bt_query_status
(*bt_component_class_source_query_method)(
		bt_self_component_class_source *comp_class,
		const bt_query_executor *query_executor,
		const char *object, const bt_value *params,
		const bt_value **result);

typedef enum bt_self_component_status
(*bt_component_class_source_accept_output_port_connection_method)(
		bt_self_component_source *self_component,
		bt_self_component_port_output *self_port,
		const bt_port_input *other_port);

typedef enum bt_self_component_status
(*bt_component_class_source_output_port_connected_method)(
		bt_self_component_source *self_component,
		bt_self_component_port_output *self_port,
		const bt_port_input *other_port);

typedef void
(*bt_component_class_source_output_port_disconnected_method)(
		bt_self_component_source *self_component,
		bt_self_component_port_output *self_port);

static inline
bt_component_class *bt_component_class_source_as_component_class(
		bt_component_class_source *comp_cls_source)
{
	return (void *) comp_cls_source;
}

extern
bt_component_class_source *bt_component_class_source_create(
		const char *name,
		bt_component_class_source_notification_iterator_next_method method);

extern int bt_component_class_source_set_init_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_init_method method);

extern int bt_component_class_source_set_finalize_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_finalize_method method);

extern int bt_component_class_source_set_accept_output_port_connection_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_accept_output_port_connection_method method);

extern int bt_component_class_source_set_output_port_connected_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_output_port_connected_method method);

extern int bt_component_class_source_set_output_port_disconnected_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_output_port_disconnected_method method);

extern int bt_component_class_source_set_query_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_query_method method);

extern int bt_component_class_source_set_notification_iterator_init_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_notification_iterator_init_method method);

extern int bt_component_class_source_set_notification_iterator_finalize_method(
		bt_component_class_source *comp_class,
		bt_component_class_source_notification_iterator_finalize_method method);

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE_GRAPH_COMPONENT_CLASS_SOURCE_H */
