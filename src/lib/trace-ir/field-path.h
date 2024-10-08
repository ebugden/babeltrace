/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016-2018 Philippe Proulx <pproulx@efficios.com>
 *
 * The Common Trace Format (CTF) Specification is available at
 * http://www.efficios.com/ctf
 */

#ifndef BABELTRACE_LIB_TRACE_IR_FIELD_PATH_H
#define BABELTRACE_LIB_TRACE_IR_FIELD_PATH_H

#include "lib/object.h"
#include <babeltrace2/trace-ir/field-path.h>
#include "common/assert.h"
#include "common/macros.h"
#include <glib.h>

struct bt_field_path_item {
	enum bt_field_path_item_type type;
	uint64_t index;
};

struct bt_field_path {
	struct bt_object base;
	enum bt_field_path_scope root;

	/* Array of `struct bt_field_path_item` (items) */
	GArray *items;
};

struct bt_field_path *bt_field_path_create(void);

static inline
struct bt_field_path_item *bt_field_path_borrow_item_by_index_inline(
		const struct bt_field_path *field_path, uint64_t index)
{
	BT_ASSERT_DBG(field_path);
	BT_ASSERT_DBG(index < field_path->items->len);
	return &bt_g_array_index(field_path->items, struct bt_field_path_item,
		index);
}

static inline
void bt_field_path_append_item(struct bt_field_path *field_path,
		struct bt_field_path_item *item)
{
	BT_ASSERT(field_path);
	BT_ASSERT(item);
	g_array_append_val(field_path->items, *item);
}

static inline
void bt_field_path_remove_last_item(struct bt_field_path *field_path)
{
	BT_ASSERT(field_path);
	BT_ASSERT(field_path->items->len > 0);
	g_array_set_size(field_path->items, field_path->items->len - 1);
}

static inline
const char *bt_field_path_item_type_string(enum bt_field_path_item_type type)
{
	switch (type) {
	case BT_FIELD_PATH_ITEM_TYPE_INDEX:
		return "INDEX";
	case BT_FIELD_PATH_ITEM_TYPE_CURRENT_ARRAY_ELEMENT:
		return "CURRENT_ARRAY_ELEMENT";
	default:
		return "(unknown)";
	}
};

#endif /* BABELTRACE_LIB_TRACE_IR_FIELD_PATH_H */
