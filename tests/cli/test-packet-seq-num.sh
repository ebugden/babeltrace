#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2015 Julien Desfossez <jdesfossez@efficios.com>
#

SH_TAP=1

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../utils/utils.sh"
fi

# shellcheck source=../utils/utils.sh
source "$UTILSSH"

NUM_TESTS=10

plan_tests $NUM_TESTS

test_no_lost() {
	local trace=$1

	"${BT_TESTS_BT2_BIN}" "$trace" >/dev/null 2>&1
	ok $? "Trace parses"
	"${BT_TESTS_BT2_BIN}" "$trace" 2>&1 >/dev/null | bt_grep "\[warning\] Tracer lost"
	if test $? = 0; then
		fail 1 "Should not find any lost events"
	else
		ok 0 "No events lost"
	fi
}

test_lost() {
	local trace=$1
	local expectedcountstr=$2

	"${BT_TESTS_BT2_BIN}" "$trace" >/dev/null 2>&1
	ok $? "Trace parses"

	# Convert warnings like:
	# WARNING: Tracer discarded 2 trace packets between ....
	# WARNING: Tracer discarded 3 trace packets between ....
	# into "2,3" and make sure it matches the expected result
	"${BT_TESTS_BT2_BIN}" "$trace" 2>&1 >/dev/null | bt_grep "WARNING: Tracer discarded" \
		| cut -d" " -f4 | tr "\n" "," | "${BT_TESTS_SED_BIN}" "s/.$//" | \
		bt_grep "$expectedcountstr" >/dev/null
	ok $? "Lost events string matches $expectedcountstr"

}

diag "Test the packet_seq_num validation"

diag "No packet lost"
test_no_lost "${BT_CTF_TRACES_PATH}/1/packet-seq-num/no-lost"

diag "No packet lost, packet_seq_num not starting at 0"
test_no_lost "${BT_CTF_TRACES_PATH}/1/packet-seq-num/no-lost-not-starting-at-0"

diag "1 stream, 2 packets lost before the last packet"
test_lost "${BT_CTF_TRACES_PATH}/1/packet-seq-num/2-lost-before-last" "2"

diag "2 streams, packets lost in one of them"
test_lost "${BT_CTF_TRACES_PATH}/1/packet-seq-num/2-streams-lost-in-1" "2"

diag "2 streams, packets lost in both"
test_lost "${BT_CTF_TRACES_PATH}/1/packet-seq-num/2-streams-lost-in-2" "2,3,1"
