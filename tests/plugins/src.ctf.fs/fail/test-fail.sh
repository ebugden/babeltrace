#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

# This test validates that a `src.ctf.fs` component handles gracefully invalid
# CTF traces and produces the expected error message.

SH_TAP=1

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../../../utils/utils.sh"
fi

# shellcheck source=../../../utils/utils.sh
source "$UTILSSH"

fail_trace_dir="$BT_CTF_TRACES_PATH/fail"

stdout_file=$(mktemp -t test-ctf-fail-stdout.XXXXXX)
stderr_file=$(mktemp -t test-ctf-fail-stderr.XXXXXX)
data_dir="${BT_TESTS_SRCDIR}/data/plugins/src.ctf.fs/fail"

test_fail() {
	local name="$1"
	local expected_stdout_file="$2"
	local expected_error_msg="$3"

	bt_cli "${stdout_file}" "${stderr_file}" \
		-c sink.text.details -p "with-trace-name=no,with-stream-name=no" "${fail_trace_dir}/${name}"
	isnt $? 0 "Trace ${name}: babeltrace exits with an error"

	bt_diff "${expected_stdout_file}" "${stdout_file}"
	ok $? "Trace ${name}: babeltrace produces the expected stdout"

	# The expected error message will likely be found in the error stream
	# even if Babeltrace aborts (e.g. hits an assert).  Check that the
	# Babeltrace CLI finishes gracefully by checking that the error stream
	# contains an error stack printed by the CLI.
	bt_grep_ok \
		"^CAUSED BY " \
		"$stderr_file" \
		"Trace $name: babeltrace produces an error stack"

	bt_grep_ok \
		"$expected_error_msg" \
		"$stderr_file" \
		"Trace $name: babeltrace produces the expected error message"
}


plan_tests 20

test_fail \
	"invalid-packet-size/trace" \
	"/dev/null" \
	"Failed to index CTF stream file '.*channel0_3'"

test_fail \
	"valid-events-then-invalid-events" \
	"${data_dir}/valid-events-then-invalid-events.expect" \
	"At 24 bits: no event record class exists with ID 255 within the data stream class with ID 0."

test_fail \
	"metadata-syntax-error" \
	"/dev/null" \
	"^  At line 3 in metadata stream: syntax error, unexpected CTF_RSBRAC: token=\"]\""

test_fail \
	"invalid-sequence-length-field-class" \
	"/dev/null" \
	"Sequence field class's length field class is not an unsigned integer field class: "

test_fail \
	"invalid-variant-selector-field-class" \
	"/dev/null" \
	"Variant field class's tag field class is not an enumeration field class: "

rm -f "${stdout_file}" "${stderr_file}"
