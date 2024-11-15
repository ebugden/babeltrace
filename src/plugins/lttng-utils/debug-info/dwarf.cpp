/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2015 Antoine Busque <abusque@efficios.com>
 *
 * Babeltrace - DWARF Information Reader
 */

#include <glib.h>
#include <stdbool.h>

#include "dwarf.hpp"

struct bt_dwarf_cu *bt_dwarf_cu_create(Dwarf *dwarf_info)
{
    struct bt_dwarf_cu *cu;

    if (!dwarf_info) {
        goto error;
    }

    cu = g_new0(struct bt_dwarf_cu, 1);
    if (!cu) {
        goto error;
    }
    cu->dwarf_info = dwarf_info;
    return cu;

error:
    return nullptr;
}

void bt_dwarf_cu_destroy(struct bt_dwarf_cu *cu)
{
    g_free(cu);
}

int bt_dwarf_cu_next(struct bt_dwarf_cu *cu)
{
    int ret;
    Dwarf_Off next_offset;
    size_t cu_header_size;

    if (!cu) {
        ret = -1;
        goto end;
    }

    ret = dwarf_nextcu(cu->dwarf_info, cu->next_offset, &next_offset, &cu_header_size, nullptr,
                       nullptr, nullptr);
    if (ret) {
        /* ret is -1 on error, 1 if no next CU. */
        goto end;
    }

    cu->offset = cu->next_offset;
    cu->next_offset = next_offset;
    cu->header_size = cu_header_size;

end:
    return ret;
}

struct bt_dwarf_die *bt_dwarf_die_create(struct bt_dwarf_cu *cu)
{
    Dwarf_Die *dwarf_die = nullptr;
    struct bt_dwarf_die *die = nullptr;

    if (!cu) {
        goto error;
    }

    dwarf_die = g_new0(Dwarf_Die, 1);
    if (!dwarf_die) {
        goto error;
    }

    dwarf_die = dwarf_offdie(cu->dwarf_info, cu->offset + cu->header_size, dwarf_die);
    if (!dwarf_die) {
        goto error;
    }

    die = g_new0(struct bt_dwarf_die, 1);
    if (!die) {
        goto error;
    }

    die->cu = cu;
    die->dwarf_die = dwarf_die;
    die->depth = 0;

    return die;

error:
    g_free(dwarf_die);
    g_free(die);
    return nullptr;
}

void bt_dwarf_die_destroy(struct bt_dwarf_die *die)
{
    if (!die) {
        return;
    }

    g_free(die->dwarf_die);
    g_free(die);
}

int bt_dwarf_die_has_children(struct bt_dwarf_die *die)
{
    return dwarf_haschildren(die->dwarf_die);
}

int bt_dwarf_die_child(struct bt_dwarf_die *die)
{
    int ret;
    Dwarf_Die *child_die = nullptr;

    if (!die) {
        ret = -1;
        goto error;
    }

    child_die = g_new0(Dwarf_Die, 1);
    if (!child_die) {
        ret = -1;
        goto error;
    }

    ret = dwarf_child(die->dwarf_die, child_die);
    if (ret) {
        /* ret is -1 on error, 1 if no child DIE. */
        goto error;
    }

    g_free(die->dwarf_die);
    die->dwarf_die = child_die;
    die->depth++;
    return 0;

error:
    g_free(child_die);
    return ret;
}

int bt_dwarf_die_next(struct bt_dwarf_die *die)
{
    int ret;
    Dwarf_Die *next_die = nullptr;

    if (!die) {
        ret = -1;
        goto error;
    }

    next_die = g_new0(Dwarf_Die, 1);
    if (!next_die) {
        ret = -1;
        goto error;
    }

    if (die->depth == 0) {
        ret = dwarf_child(die->dwarf_die, next_die);
        if (ret) {
            /* ret is -1 on error, 1 if no child DIE. */
            goto error;
        }

        die->depth = 1;
    } else {
        ret = dwarf_siblingof(die->dwarf_die, next_die);
        if (ret) {
            /* ret is -1 on error, 1 if we reached end of
             * DIEs at this depth. */
            goto error;
        }
    }

    g_free(die->dwarf_die);
    die->dwarf_die = next_die;
    return 0;

error:
    g_free(next_die);
    return ret;
}

int bt_dwarf_die_get_tag(struct bt_dwarf_die *die, int *tag)
{
    int _tag;

    if (!die || !tag) {
        goto error;
    }

    _tag = dwarf_tag(die->dwarf_die);
    if (_tag == DW_TAG_invalid) {
        goto error;
    }

    *tag = _tag;
    return 0;

error:
    return -1;
}

int bt_dwarf_die_get_name(struct bt_dwarf_die *die, char **name)
{
    const char *_name;

    if (!die || !name) {
        goto error;
    }

    _name = dwarf_diename(die->dwarf_die);
    if (!_name) {
        goto error;
    }

    *name = g_strdup(_name);
    if (!*name) {
        goto error;
    }

    return 0;

error:
    return -1;
}

int bt_dwarf_die_get_call_file(struct bt_dwarf_die *die, char **filename)
{
    int ret;
    Dwarf_Sword file_no;
    const char *_filename = nullptr;
    Dwarf_Files *src_files = nullptr;
    Dwarf_Attribute *file_attr = nullptr;
    struct bt_dwarf_die *cu_die = nullptr;

    if (!die || !filename) {
        goto error;
    }

    file_attr = g_new0(Dwarf_Attribute, 1);
    if (!file_attr) {
        goto error;
    }

    file_attr = dwarf_attr(die->dwarf_die, DW_AT_call_file, file_attr);
    if (!file_attr) {
        goto error;
    }

    ret = dwarf_formsdata(file_attr, &file_no);
    if (ret) {
        goto error;
    }

    cu_die = bt_dwarf_die_create(die->cu);
    if (!cu_die) {
        goto error;
    }

    ret = dwarf_getsrcfiles(cu_die->dwarf_die, &src_files, nullptr);
    if (ret) {
        goto error;
    }

    _filename = dwarf_filesrc(src_files, file_no, nullptr, nullptr);
    if (!_filename) {
        goto error;
    }

    *filename = g_strdup(_filename);

    bt_dwarf_die_destroy(cu_die);
    g_free(file_attr);

    return 0;

error:
    bt_dwarf_die_destroy(cu_die);
    g_free(file_attr);

    return -1;
}

int bt_dwarf_die_get_call_line(struct bt_dwarf_die *die, uint64_t *line_no)
{
    int ret = 0;
    Dwarf_Attribute *line_attr = nullptr;
    uint64_t _line_no;

    if (!die || !line_no) {
        goto error;
    }

    line_attr = g_new0(Dwarf_Attribute, 1);
    if (!line_attr) {
        goto error;
    }

    line_attr = dwarf_attr(die->dwarf_die, DW_AT_call_line, line_attr);
    if (!line_attr) {
        goto error;
    }

    ret = dwarf_formudata(line_attr, &_line_no);
    if (ret) {
        goto error;
    }

    *line_no = _line_no;
    g_free(line_attr);

    return 0;

error:
    g_free(line_attr);

    return -1;
}

int bt_dwarf_die_contains_addr(struct bt_dwarf_die *die, uint64_t addr, bool *contains)
{
    int ret;

    ret = dwarf_haspc(die->dwarf_die, addr);
    if (ret == -1) {
        goto error;
    }

    *contains = (ret == 1);

    return 0;

error:
    return -1;
}
