/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2015 EfficiOS Inc. and Linux Foundation
 * Copyright (C) 2015 Antoine Busque <abusque@efficios.com>
 *
 * Babeltrace bt_dwarf (DWARF utilities) tests
 */

#include <lttng-utils/debug-info/dwarf.hpp>

#include <fcntl.h>
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tap/tap.h"

#define NR_TESTS 17

#define SO_NAME        "libhello-so"
#define DWARF_DIR_NAME "dwarf-full"
#define ELF_DIR_NAME   "elf-only"

/*
 * Test that we fail on an ELF file without DWARF.
 */
static void test_bt_no_dwarf(const char *data_dir)
{
    int fd;
    char *path;
    Dwarf *dwarf_info = NULL;

    path = g_build_filename(data_dir, ELF_DIR_NAME, SO_NAME, NULL);
    if (!path) {
        diag("Failed to allocate memory for path");
        exit(EXIT_FAILURE);
    }

    fd = open(path, O_RDONLY);
    ok(fd >= 0, "Open ELF file %s", path);
    if (fd < 0) {
        skip(1, "dwarf_begin failed as expected");
    } else {
        dwarf_info = dwarf_begin(fd, DWARF_C_READ);
        ok(!dwarf_info, "dwarf_begin failed as expected");
    }

    if (dwarf_info) {
        dwarf_end(dwarf_info);
    }

    if (fd >= 0) {
        close(fd);
    }
    g_free(path);
}

/*
 * Test with a proper ELF file with DWARF.
 */
static void test_bt_dwarf(const char *data_dir)
{
    int fd, ret, tag = -1;
    char *path;
    char *die_name = NULL;
    struct bt_dwarf_cu *cu = NULL;
    struct bt_dwarf_die *die = NULL;
    Dwarf *dwarf_info = NULL;

    path = g_build_filename(data_dir, DWARF_DIR_NAME, SO_NAME, NULL);
    if (!path) {
        diag("Failed to allocate memory for path");
        exit(EXIT_FAILURE);
    }

    fd = open(path, O_RDONLY);
    ok(fd >= 0, "Open DWARF file %s", path);
    if (fd < 0) {
        exit(EXIT_FAILURE);
    }
    dwarf_info = dwarf_begin(fd, DWARF_C_READ);
    ok(dwarf_info, "dwarf_begin successful");
    cu = bt_dwarf_cu_create(dwarf_info);
    ok(cu, "bt_dwarf_cu_create successful");
    ret = bt_dwarf_cu_next(cu);
    ok(ret == 0, "bt_dwarf_cu_next successful");
    die = bt_dwarf_die_create(cu);
    ok(die, "bt_dwarf_die_create successful");
    if (!die) {
        exit(EXIT_FAILURE);
    }
    /*
	 * Test bt_dwarf_die_next twice, as the code path is different
	 * for DIEs at depth 0 (just created) and other depths.
	 */
    ret = bt_dwarf_die_next(die);
    ok(ret == 0, "bt_dwarf_die_next from root DIE successful");
    ok(die->depth == 1, "bt_dwarf_die_next from root DIE - correct depth value");
    ret = bt_dwarf_die_next(die);
    ok(ret == 0, "bt_dwarf_die_next from non-root DIE successful");
    ok(die->depth == 1, "bt_dwarf_die_next from non-root DIE - correct depth value");

    /* Reset DIE to test dwarf_child */
    bt_dwarf_die_destroy(die);
    die = bt_dwarf_die_create(cu);
    if (!die) {
        diag("Failed to create bt_dwarf_die");
        exit(EXIT_FAILURE);
    }

    ret = bt_dwarf_die_child(die);
    ok(ret == 0, "bt_dwarf_die_child successful");
    ok(die->depth == 1, "bt_dwarf_die_child - correct depth value");

    ret = bt_dwarf_die_get_tag(die, &tag);
    ok(ret == 0, "bt_dwarf_die_get_tag successful");
    ok(tag == DW_TAG_typedef, "bt_dwarf_die_get_tag - correct tag value");
    ret = bt_dwarf_die_get_name(die, &die_name);
    ok(ret == 0, "bt_dwarf_die_get_name successful");
    ok(strcmp(die_name, "size_t") == 0, "bt_dwarf_die_get_name - correct name value");

    bt_dwarf_die_destroy(die);
    bt_dwarf_cu_destroy(cu);
    dwarf_end(dwarf_info);
    free(die_name);
    close(fd);
    g_free(path);
}

int main(int argc, char **argv)
{
    const char *data_dir;

    plan_tests(NR_TESTS);

    if (argc != 2) {
        return EXIT_FAILURE;
    } else {
        data_dir = argv[1];
    }

    test_bt_no_dwarf(data_dir);
    test_bt_dwarf(data_dir);

    return exit_status();
}
