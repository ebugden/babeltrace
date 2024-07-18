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

plan_tests 20

stdout=$(mktemp -t test-intersection-stdout.XXXXXX)
stderr=$(mktemp -t test-intersection-stderr.XXXXXX)

test_intersect() {
	local trace="$1"
	local totalevents="$2"
	local intersect="$3"

	local cnt

	bt_cli "${stdout}" "/dev/null" "${trace}"
	ok $? "run without --stream-intersection"

	cnt=$(wc -l < "${stdout}")
	test "${cnt// /}" = "$totalevents"
	ok $? "$totalevents events in the whole trace"

	bt_cli "${stdout}" "/dev/null" --stream-intersection "${trace}"
	ok $? "run with --stream-intersection"

	cnt=$(wc -l < "${stdout}")
	test "${cnt// /}" = "$intersect"
	ok $? "$intersect events in streams intersecting"
}

test_intersect_fails() {
	local trace="$1"
	local totalevents="$2"
	local expected_error_message="$3"

	bt_cli "${stdout}" "/dev/null" "${trace}"
	ok $? "run without --stream-intersection"

	cnt=$(wc -l < "${stdout}")
	test "${cnt// /}" = "$totalevents"
	ok $? "$totalevents events in the whole trace"

	bt_cli "${stdout}" "${stderr}" --stream-intersection "${trace}"
	isnt "$?" 0 "run with --stream-intersection fails"

	bt_grep_ok \
		"${expected_error_message}" \
		"${stderr}" \
		"stderr contains expected error message"
}

diag "Test the stream intersection feature"

diag "2 streams offsetted with 3 packets intersecting"
test_intersect "${BT_CTF_TRACES_PATH}/1/intersection/3eventsintersect" 8 3

diag "2 streams offsetted with 3 packets intersecting (exchanged file names)"
test_intersect "${BT_CTF_TRACES_PATH}/1/intersection/3eventsintersectreverse" 8 3

diag "Only 1 stream"
test_intersect "${BT_CTF_TRACES_PATH}/1/intersection/onestream" 3 3

diag "No intersection between 2 streams"
test_intersect_fails "${BT_CTF_TRACES_PATH}/1/intersection/nointersect" 6 \
	"Trimming time range's beginning time is greater than end time: "

diag "No stream at all"
test_intersect_fails "${BT_CTF_TRACES_PATH}/1/intersection/nostream" 0 \
	"Trace has no streams: "

rm -f "${stdout}" "${stderr}"
