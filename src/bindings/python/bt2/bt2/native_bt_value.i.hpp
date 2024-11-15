/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2016 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_VALUE_I_HPP
#define BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_VALUE_I_HPP

struct bt_value_map_get_keys_data
{
    struct bt_value *keys;
};

static bt_value_map_foreach_entry_const_func_status
bt_value_map_get_keys_cb(const char *key, const struct bt_value *object, void *data)
{
    const auto priv_data = static_cast<bt_value_map_get_keys_data *>(data);
    bt_value_array_append_element_status status =
        bt_value_array_append_string_element(priv_data->keys, key);

    BT_ASSERT(status == BT_VALUE_ARRAY_APPEND_ELEMENT_STATUS_OK ||
              status == BT_VALUE_ARRAY_APPEND_ELEMENT_STATUS_MEMORY_ERROR);
    return static_cast<bt_value_map_foreach_entry_const_func_status>(status);
}

static struct bt_value *bt_value_map_get_keys(const struct bt_value *map_obj)
{
    bt_value_map_foreach_entry_const_status status;
    struct bt_value_map_get_keys_data data;

    data.keys = bt_value_array_create();
    if (!data.keys) {
        return NULL;
    }

    status = bt_value_map_foreach_entry_const(map_obj, bt_value_map_get_keys_cb, &data);
    if (status != __BT_FUNC_STATUS_OK) {
        goto error;
    }

    goto end;

error:
    if (data.keys) {
        BT_VALUE_PUT_REF_AND_RESET(data.keys);
    }

end:
    return data.keys;
}

#endif /* BABELTRACE_BINDINGS_PYTHON_BT2_BT2_NATIVE_BT_VALUE_I_HPP */
