#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2020 Philippe Proulx <pproulx@efficios.com>

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../../utils/utils.sh"
fi

# shellcheck source=../../utils/utils.sh
source "$UTILSSH"

reldir=lib/conds
export BT_TESTS_LIB_CONDS_TRIGGER_BIN="$BT_TESTS_BUILDDIR/$reldir/conds-triggers"

if [ "$BT_TESTS_OS_TYPE" = "mingw" ]; then
	BT_TESTS_LIB_CONDS_TRIGGER_BIN="$BT_TESTS_LIB_CONDS_TRIGGER_BIN.exe"
fi

bt_run_py_test "$BT_TESTS_SRCDIR/$reldir" test.py $*
