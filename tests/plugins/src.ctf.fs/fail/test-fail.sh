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

stdout_file=$(mktemp -t test-ctf-fail-stdout.XXXXXX)
stderr_file=$(mktemp -t test-ctf-fail-stderr.XXXXXX)
data_dir="${BT_TESTS_SRCDIR}/data/plugins/src.ctf.fs/fail"

# Parameters: <trace-name> <ctf-version>
fail_trace_path() {
	local name="$1"
	local ctf_version="$2"

	echo "$BT_CTF_TRACES_PATH/$ctf_version/fail/$name"
}

# Parameters: <trace-name> <ctf-version> <method> <expected-stdout-file> <expected-error-msg>
#
# <method> can be either "autodisc" or "component".  "autodisc" passes the trace
# path directly to babeltrace2, making it use the auto-discovery mechanism.
# "component" instantiates a `src.ctf.fs` component explicitly.
test_fail_method() {
	local name="$1"
	local ctf_version="$2"
	local method="$3"
	local expected_stdout_file="$4"
	local expected_error_msg="$5"
	local trace_path

	trace_path=$(fail_trace_path "$name" "$ctf_version")

	if [ "$method" = "autodisc" ]; then
		bt_cli "${stdout_file}" "${stderr_file}" \
			-c sink.text.details -p "with-trace-name=no,with-stream-name=no" "$trace_path"
	elif [ "$method" = "component" ]; then
		bt_cli "${stdout_file}" "${stderr_file}" \
			-c sink.text.details -p "with-trace-name=no,with-stream-name=no" -c src.ctf.fs -p "inputs=[\"$(maybe_cygpath_m "$trace_path")\"]"
	else
		echo "invalid method: $method"
		exit 1
	fi
	isnt $? 0 "Trace ${name}: method $method: babeltrace exits with an error"

	bt_diff "${expected_stdout_file}" "${stdout_file}"
	ok $? "Trace ${name}: method $method: babeltrace produces the expected stdout"

	# The expected error message will likely be found in the error stream
	# even if Babeltrace aborts (e.g. hits an assert).  Check that the
	# Babeltrace CLI finishes gracefully by checking that the error stream
	# contains an error stack printed by the CLI.
	bt_grep_ok \
		"^CAUSED BY " \
		"$stderr_file" \
		"Trace ${name}: method $method: babeltrace produces an error stack"

	bt_grep_ok \
		"$expected_error_msg" \
		"$stderr_file" \
		"Trace ${name}: method $method: babeltrace produces the expected error message"
}

# Parameters: <trace-name> <ctf-version> <expected-stdout-file> <expected-error-msg>
test_fail() {
	local name="$1"
	local ctf_version="$2"
	local expected_stdout_file="$3"
	local expected_error_msg="$4"
	for method in autodisc component; do
		test_fail_method "$name" "$ctf_version" "$method" \
			"$expected_stdout_file" "$expected_error_msg"
	done
}

plan_tests 40

test_fail \
	"invalid-packet-size/trace" \
	1 \
	"/dev/null" \
	"Failed to index CTF stream file '.*channel0_3'"

test_fail \
	"valid-events-then-invalid-events/trace" \
	1 \
	"${data_dir}/valid-events-then-invalid-events.expect" \
	"At 24 bits: no event record class exists with ID 255 within the data stream class with ID 0."

test_fail \
	"metadata-syntax-error" \
	1 \
	"/dev/null" \
	"^  At line 3 in metadata stream: syntax error, unexpected CTF_RSBRAC: token=\"]\""

test_fail \
	"invalid-sequence-length-field-class" \
	 1 \
	"/dev/null" \
	"Sequence field class's length field class is not an unsigned integer field class: "

test_fail \
	"invalid-variant-selector-field-class" \
	 1 \
	"/dev/null" \
	"Variant field class's tag field class is not an enumeration field class: "

rm -f "${stdout_file}" "${stderr_file}"
