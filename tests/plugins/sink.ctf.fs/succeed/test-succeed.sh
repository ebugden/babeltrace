#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 Philippe Proulx <pproulx@efficios.com>
#

# This test validates that a `src.ctf.fs` component successfully reads
# specific CTF traces and creates the expected messages.
#
# Such CTF traces to open either exist (in `tests/ctf-traces/1/succeed`)
# or are generated by this test using local trace generators.

SH_TAP=1

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../../../utils/utils.sh"
fi

# shellcheck source=../../../utils/utils.sh
source "$UTILSSH"

this_dir_relative="plugins/sink.ctf.fs/succeed"
this_dir_build="$BT_TESTS_BUILDDIR/$this_dir_relative"
expect_dir="$BT_TESTS_DATADIR/$this_dir_relative"
succeed_traces="$BT_CTF_TRACES_PATH/1/succeed"

test_ctf_single() {
	local name="$1"
	local in_trace_dir="$2"
	local temp_out_trace_dir

	temp_out_trace_dir="$(mktemp -d)"

	diag "Converting trace '$name' to CTF through 'sink.ctf.fs'"
	"$BT_TESTS_BT2_BIN" >/dev/null "$in_trace_dir" -o ctf -w "$temp_out_trace_dir"
	ret=$?
	ok $ret "'sink.ctf.fs' component succeeds with input trace '$name'"
	converted_test_name="Converted trace '$name' gives the expected output"

	if [ $ret -eq 0 ]; then
		bt_diff_details_ctf_single "$expect_dir/trace-$name.expect" \
			"$temp_out_trace_dir" \
			'-p' 'with-uuid=no,with-trace-name=no,with-stream-name=no'
		ok $? "$converted_test_name"
	else
		fail "$converted_test_name"
	fi

	rm -rf "$temp_out_trace_dir"
}

test_ctf_existing_single() {
	local name="$1"
	local trace_dir="$succeed_traces/$name"

	test_ctf_single "$name" "$trace_dir"
}

test_ctf_gen_single() {
	local name="$1"
	local temp_gen_trace_dir

	temp_gen_trace_dir="$(mktemp -d)"

	diag "Generating trace '$name'"

	if ! "$this_dir_build/gen-trace-$name" "$temp_gen_trace_dir"; then
		# this is not part of the test itself; it must not fail
		echo "ERROR: \"$this_dir_build/gen-trace-$name" "$temp_gen_trace_dir\" failed" >&2
		rm -rf "$temp_gen_trace_dir"
		exit 1
	fi

	test_ctf_single "$name" "$temp_gen_trace_dir"
	rm -rf "$temp_gen_trace_dir"
}

plan_tests 14

test_ctf_gen_single float
test_ctf_gen_single double
test_ctf_existing_single "meta-variant-no-underscore"
test_ctf_existing_single "meta-variant-one-underscore"
test_ctf_existing_single "meta-variant-reserved-keywords"
test_ctf_existing_single "meta-variant-same-with-underscore"
test_ctf_existing_single "meta-variant-two-underscores"
