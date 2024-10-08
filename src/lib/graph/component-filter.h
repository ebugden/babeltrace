/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 */

#ifndef BABELTRACE_LIB_GRAPH_COMPONENT_FILTER_H
#define BABELTRACE_LIB_GRAPH_COMPONENT_FILTER_H

#include <babeltrace2/graph/component.h>

#include "component.h"

struct bt_component_filter {
	struct bt_component parent;
};

struct bt_component *bt_component_filter_create(void);

#endif /* BABELTRACE_LIB_GRAPH_COMPONENT_FILTER_H */
