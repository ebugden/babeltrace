/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2017 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 */

#ifndef BABELTRACE_LIB_GRAPH_GRAPH_H
#define BABELTRACE_LIB_GRAPH_GRAPH_H

/* Protection: this file uses BT_LIB_LOG*() macros directly */
#ifndef BT_LIB_LOG_SUPPORTED
# error Please include "lib/logging.h" before including this file.
#endif

#include <babeltrace2/graph/graph.h>
#include <babeltrace2/graph/message.h>
#include "lib/object.h"
#include "lib/object-pool.h"
#include "common/assert.h"
#include <stdbool.h>
#include <glib.h>

#include "component.h"
#include "component-sink.h"
#include "connection.h"

/* Protection: this file uses BT_LIB_LOG*() macros directly */
#ifndef BT_LIB_LOG_SUPPORTED
# error Please include "lib/logging.h" before including this file.
#endif

struct bt_component;
struct bt_port;

enum bt_graph_configuration_state {
	BT_GRAPH_CONFIGURATION_STATE_CONFIGURING,
	BT_GRAPH_CONFIGURATION_STATE_PARTIALLY_CONFIGURED,
	BT_GRAPH_CONFIGURATION_STATE_CONFIGURED,
	BT_GRAPH_CONFIGURATION_STATE_FAULTY,
	BT_GRAPH_CONFIGURATION_STATE_DESTROYING,
};

struct bt_graph {
	/**
	 * A component graph contains components and point-to-point connection
	 * between these components.
	 *
	 * In terms of ownership:
	 * 1) The graph is the components' parent,
	 * 2) The graph is the connections' parent,
	 * 3) Components share the ownership of their connections,
	 * 4) A connection holds weak references to its two component endpoints.
	 */
	struct bt_object base;

	/* Array of pointers to bt_connection. */
	GPtrArray *connections;
	/* Array of pointers to bt_component. */
	GPtrArray *components;
	/* Queue of pointers (weak references) to sink bt_components. */
	GQueue *sinks_to_consume;

	uint64_t mip_version;

	/*
	 * Array of `struct bt_interrupter *`, each one owned by this.
	 * If any interrupter is set, then this graph is deemed
	 * interrupted.
	 */
	GPtrArray *interrupters;

	/*
	 * Default interrupter, owned by this.
	 */
	struct bt_interrupter *default_interrupter;

	bool has_sink;

	/*
	 * If this is false, then the public API's consuming
	 * functions (bt_graph_consume() and bt_graph_run()) return
	 * BT_FUNC_STATUS_CANNOT_CONSUME. The internal "no check"
	 * functions always work.
	 *
	 * In bt_port_output_message_iterator_create(), on success,
	 * this flag is cleared so that the iterator remains the only
	 * consumer for the graph's lifetime.
	 */
	bool can_consume;

	enum bt_graph_configuration_state config_state;

	struct {
		GArray *source_output_port_added;
		GArray *filter_output_port_added;
		GArray *filter_input_port_added;
		GArray *sink_input_port_added;
	} listeners;

	/* Pool of `struct bt_message_event *` */
	struct bt_object_pool event_msg_pool;

	/* Pool of `struct bt_message_packet_beginning *` */
	struct bt_object_pool packet_begin_msg_pool;

	/* Pool of `struct bt_message_packet_end *` */
	struct bt_object_pool packet_end_msg_pool;

	/*
	 * Array of `struct bt_message *` (weak).
	 *
	 * This is an array of all the messages ever created from
	 * this graph. Some of them can be in one of the pools above,
	 * some of them can be at large. Because each message has a
	 * weak pointer to the graph containing its pool, we need to
	 * notify each message that the graph is gone on graph
	 * destruction.
	 *
	 * TODO: When we support a maximum size for object pools,
	 * add a way for a message to remove itself from this
	 * array (on destruction).
	 */
	GPtrArray *messages;
};

static inline
void bt_graph_set_can_consume(struct bt_graph *graph, bool can_consume)
{
	BT_ASSERT_DBG(graph);
	graph->can_consume = can_consume;
}

int bt_graph_consume_sink_no_check(struct bt_graph *graph,
		struct bt_component_sink *sink);

enum bt_graph_listener_func_status bt_graph_notify_port_added(struct bt_graph *graph,
		struct bt_port *port);

void bt_graph_remove_connection(struct bt_graph *graph,
		struct bt_connection *connection);

void bt_graph_add_message(struct bt_graph *graph,
		struct bt_message *msg);

bool bt_graph_is_interrupted(const struct bt_graph *graph);

static inline
const char *bt_graph_configuration_state_string(
		enum bt_graph_configuration_state state)
{
	switch (state) {
	case BT_GRAPH_CONFIGURATION_STATE_CONFIGURING:
		return "CONFIGURING";
	case BT_GRAPH_CONFIGURATION_STATE_PARTIALLY_CONFIGURED:
		return "PARTIALLY_CONFIGURED";
	case BT_GRAPH_CONFIGURATION_STATE_CONFIGURED:
		return "CONFIGURED";
	case BT_GRAPH_CONFIGURATION_STATE_FAULTY:
		return "FAULTY";
	case BT_GRAPH_CONFIGURATION_STATE_DESTROYING:
		return "DESTROYING";
	default:
		return "(unknown)";
	}
}

static inline
void bt_graph_make_faulty(struct bt_graph *graph)
{
	graph->config_state = BT_GRAPH_CONFIGURATION_STATE_FAULTY;
	BT_LIB_LOGI("Set graph's state to faulty: %![graph-]+g", graph);
}

#endif /* BABELTRACE_LIB_GRAPH_GRAPH_H */
