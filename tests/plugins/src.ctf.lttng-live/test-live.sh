#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 Philippe Proulx <pproulx@efficios.com>
#

# This test validates that a `src.ctf.lttng-live` component successfully does
# various tasks that a `src.ctf.lttng-live` component is expected to do, like
# listing tracing sessions and receiving live traces / producing the expected
# messages out of it.
#
# A mock LTTng live server is used to feed data to the component.

SH_TAP=1

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../../utils/utils.sh"
fi

# shellcheck source=../../utils/utils.sh
source "$UTILSSH"

function cleanup ()
{
	# Disable trap for SIGTERM since the following kill to the
	# pidgroup will be SIGTERM. Otherwise it loops.
	# The '-' before the pid number ($$) indicates 'kill' to signal the
	# whole process group.
	trap - SIGTERM && kill -- -$$
}

# Ensure that background child jobs are killed on SIGINT/SIGTERM
trap cleanup SIGINT SIGTERM

this_dir_relative="plugins/src.ctf.lttng-live"
test_data_dir="$BT_TESTS_DATADIR/$this_dir_relative"
trace_dir="$BT_CTF_TRACES_PATH"

if [ "$BT_TESTS_OS_TYPE" = "mingw" ]; then
	# Same as the above, but in Windows form (C:\foo\bar) instead of Unix form
	# (/c/foo/bar).
	trace_dir_native=$(cygpath -w "${trace_dir}")
else
	trace_dir_native="${trace_dir}"
fi

lttng_live_server() {
	local pid_file="$1"
	local retcode_file="$2"
	shift 2
	local server_args=("$@")

	local server_script="$test_data_dir/lttng_live_server.py"

	# start server
	diag "$BT_TESTS_PYTHON_BIN $server_script ${server_args[*]}"
	bt_run_in_py_utils_env "$BT_TESTS_PYTHON_BIN" "$server_script" "${server_args[@]}" 1>&2 &

	# write PID to file
	echo $! > "$pid_file"

	# wait for server to exit
	wait

	# write return code to file
	echo $? > "$retcode_file"
}

kill_lttng_live_server() {
	local pid_file="$1"

	if [ ! -s "$pid_file" ]; then
		return
	fi

	kill -9 "$(cat "$pid_file")"
}

get_cli_output_with_lttng_live_server() {
	local cli_args_template="$1"
	local cli_stdout_file="$2"
	local cli_stderr_file="$3"
	local port_file="$4"
	local trace_path_prefix="$5"
	shift 5
	local server_args=("$@")

	local i
	local ret
	local port
	local cli_args
	local server_pid_file
	local server_retcode_file

	server_args+=(--port-file "$port_file" --trace-path-prefix "$trace_path_prefix")
	server_pid_file="$(mktemp -t test-live-server-pid.XXXXXX)"
	server_retcode_file="$(mktemp -t test-live-server-ret.XXXXX)"

	diag "Starting LTTng live server mockup"

	# This starts the server, which eventually writes its listening
	# port number to the `$port_file` file. The lttng_live_server()
	# function itself writes the server's PID to the
	# `$server_pid_file` file. When the server exits,
	# lttng_live_server() writes its return code to the
	# `$server_retcode_file` file.
	lttng_live_server "$server_pid_file" "$server_retcode_file" "${server_args[@]}" &

	# Get port number
	i=0
	while [ ! -s "$port_file" ]; do
		sleep .1

		# Timeout of 30 seconds
		if [ "$i" -eq "300" ]; then
			# too long, kill it
			kill_lttng_live_server "$server_pid_file"
			wait
			rm -f "$server_pid_file"
			rm -f "$server_retcode_file"
			return 1
		fi

		i=$((i + 1))
	done

	port=$(<"$port_file")

	diag "LTTng live port is $port"

	cli_args=${cli_args_template//@PORT@/$port}

	# Split argument string by spaces into an array.
	IFS=' ' read -ra cli_args <<< "$cli_args"

	if ! bt_cli "$cli_stdout_file" "$cli_stderr_file" "${cli_args[@]}"; then
		# CLI failed: cancel everything else
		kill_lttng_live_server "$server_pid_file"
		wait
		rm -f "$server_pid_file"
		rm -f "$server_retcode_file"
		return 1
	fi

	# get server's return code
	i=0
	while [ ! -s "$server_retcode_file" ]; do
		sleep .1

		# Timeout of 30 seconds
		if [ "$i" -eq "300" ]; then
			# too long, kill it
			kill_lttng_live_server "$server_pid_file"
			wait
			rm -f "$server_pid_file"
			rm -f "$server_retcode_file"
			return 1
		fi

		i=$((i + 1))
	done

	wait

	ret=$(<"$server_retcode_file")

	rm -f "$server_pid_file"
	rm -f "$server_retcode_file"
	return "$ret"
}

run_test() {
	local test_text="$1"
	local cli_args_template="$2"
	local expected_stdout="$3"
	local expected_stderr="$4"
	local trace_path_prefix="$5"
	shift 5
	local server_args=("$@")

	local cli_stderr
	local cli_stdout
	local port_file
	local port

	cli_stderr="$(mktemp -t test-live-stderr.XXXXXX)"
	cli_stdout="$(mktemp -t test-live-stdout.XXXXXX)"
	port_file="$(mktemp -t test-live-server-port.XXXXXX)"

	get_cli_output_with_lttng_live_server "$cli_args_template" "$cli_stdout" \
		"$cli_stderr" "$port_file" "$trace_path_prefix" "${server_args[@]}"
	port=$(<"$port_file")

	bt_diff "$expected_stdout" "$cli_stdout"
	ok $? "$test_text - stdout"
	bt_diff "$expected_stderr" "$cli_stderr"
	ok $? "$test_text - stderr"

	rm -f "$cli_stderr"
	rm -f "$cli_stdout"
	rm -f "$port_file"
}

test_list_sessions() {
	# Test the basic listing of sessions.
	# Ensure that a multi-domain trace is seen as a single session.
	# run_test() is not used here because the port is needed to craft the
	# expected output.

	local port
	local port_file
	local tmp_stdout_expected
	local template_expected

	local test_text="CLI prints the expected session list"
	local cli_args_template="-i lttng-live net://localhost:@PORT@"
	local server_args=("$test_data_dir/list-sessions.json")

	template_expected=$(<"$test_data_dir/cli-list-sessions.expect")
	cli_stderr="$(mktemp -t test-live-list-sessions-stderr.XXXXXX)"
	cli_stdout="$(mktemp -t test-live-list-sessions-stdout.XXXXXX)"
	port_file="$(mktemp -t test-live-list-sessions-server-port.XXXXXX)"
	tmp_stdout_expected="$(mktemp -t test-live-list-sessions-stdout-expected.XXXXXX)"

	get_cli_output_with_lttng_live_server "$cli_args_template" "$cli_stdout" \
		"$cli_stderr" "$port_file" "$trace_dir_native" "${server_args[@]}"
	port=$(<"$port_file")

	# Craft the expected output. This is necessary since the port number
	# (random) of a "relayd" is present in the output.
	template_expected=${template_expected//@PORT@/$port}

	echo "$template_expected" > "$tmp_stdout_expected"

	bt_diff "$tmp_stdout_expected" "$cli_stdout"
	ok $? "$test_text - stdout"
	bt_diff "/dev/null" "$cli_stderr"
	ok $? "$test_text - stderr"

	rm -f "$cli_stderr"
	rm -f "$cli_stdout"
	rm -f "$port_file"
	rm -f "$tmp_stdout_expected"
}

test_base() {
	# Attach and consume data from a multi packets ust session with no
	# discarded events.
	local test_text="CLI attach and fetch from single-domains session - no discarded events"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/trace-with-index -c sink.text.details"
	local server_args=("$test_data_dir/base.json")
	local expected_stdout="${test_data_dir}/cli-base.expect"
	local expected_stderr="/dev/null"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
}

test_multi_domains() {
	# Attach and consume data from a multi-domains session with discarded
	# events.
	local test_text="CLI attach and fetch from multi-domains session - discarded events"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/multi-domains -c sink.text.details"
	local server_args=("${test_data_dir}/multi-domains.json")
	local expected_stdout="$test_data_dir/cli-multi-domains.expect"
	local expected_stderr="/dev/null"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
}

test_rate_limited() {
	# Attach and consume data from a multi packets ust session with no
	# discarded events. Enforce a server side limit on the stream data
	# requests size. Ensure that babeltrace respect the returned size and that
	# many requests per packet works as expected.
	# The packet size of the test trace is 4k. Limit requests to 1k.
	local test_text="CLI many requests per packet"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/trace-with-index -c sink.text.details"
	local server_args=(--max-query-data-response-size 1024 "$test_data_dir/rate-limited.json")
	local expected_stdout="${test_data_dir}/cli-base.expect"
	local expected_stderr="/dev/null"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
}

test_compare_to_ctf_fs() {
	# Compare the details text sink or ctf.fs and ctf.lttng-live to ensure
	# that the trace is parsed the same way.
	# Do the same with the session swapped on the relayd side. This validate
	# that ordering is consistent between live and ctf fs.
	local test_text="CLI src.ctf.fs vs src.ctf.lttng-live"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/multi-domains -c sink.text.details --params with-trace-name=false,with-stream-name=false"
	local server_args=("$test_data_dir/multi-domains.json")
	local server_args_inverse=("$test_data_dir/multi-domains-inverse.json")
	local expected_stdout
	local expected_stderr

	expected_stdout="$(mktemp -t test-live-compare-stdout-expected.XXXXXX)"
	expected_stderr="$(mktemp -t test-live-compare-stderr-expected.XXXXXX)"

	bt_cli "$expected_stdout" "$expected_stderr" "${trace_dir}/succeed/multi-domains" -c sink.text.details --params "with-trace-name=false,with-stream-name=false"
	bt_remove_cr "${expected_stdout}"
	bt_remove_cr "${expected_stderr}"

	# Hack. To be removed when src.ctf.lttng-live is updated to use the new
	# IR generator.
	"$BT_TESTS_SED_BIN" -i '/User attributes:/d' "${expected_stdout}"
	"$BT_TESTS_SED_BIN" -i '/babeltrace.org,2020:/d' "${expected_stdout}"
	"$BT_TESTS_SED_BIN" -i '/log-level: warning/d' "${expected_stdout}"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
	diag "Inverse session order from lttng-relayd"
	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args_inverse[@]}"

	rm -f "$expected_stdout"
	rm -f "$expected_stderr"
}

test_inactivity_discarded_packet() {
	# Attach and consume data from a multi-packet trace with discarded
	# packets and emit an inactivity beacon during the discarded packets
	# period.
	#
	# | pkt seq:0 |<-------discarded packets------>| pkt seq:8 |
	# 0          20                                121       140
	#
	# This test was introduced to cover the following bug:
	#
	# When reading this type of trace locally, the CTF source is expected
	# to introduce a "Discarded packets" message between packets 0 and 8.
	# The timestamps of this message are [pkt0.end_ts, pkt8.begin_ts].
	#
	# In the context of a live source, the tracer could report an inactivity
	# period during the interval of the "Discarded packets" message.
	# Those messages eventually translate into a
	# "Iterator inactivity" message with a timestamp set at the end of the
	# inactivity period.
	#
	# If the tracer reports an inactivity period that ends at a point
	# between pkt0 and pkt7 (resulting in an "Iterator inactivity" message),
	# the live source could send a "Discarded packets" message that starts
	# before the preceding "Iterator inactivity" message. This would break
	# the monotonicity constraint of the graph.
	local test_text="CLI attach and fetch from single-domains session - inactivity discarded packet"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/7-lost-between-2-with-index -c sink.text.details"
	local server_args=("$test_data_dir/inactivity-discarded-packet.json")
	local expected_stdout="$test_data_dir/inactivity-discarded-packet.expect"
	local expected_stderr="/dev/null"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
}

test_split_metadata() {
	# Consume a metadata stream sent in two parts. This testcase tests the
	# behaviour of Babeltrace when the tracing session was cleared (lttng
	# clear) but the metadata is not yet available to the relay. In such
	# cases, when asked for metadata, the relay will return the
	# `LTTNG_VIEWER_METADATA_OK` status and a data length of 0. The viewer
	# need to consider such case as a request to retry fetching metadata.
	#
	# This testcase emulates such behaviour by adding empty metadata
	# packets.

	local test_text="CLI attach and fetch from single-domain session - Receive metadata in two sections separated by a empty section"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/split-metadata -c sink.text.details"
	local server_args=("$test_data_dir/split-metadata.json")
	local expected_stdout="${test_data_dir}/split-metadata.expect"
	local expected_stderr="/dev/null"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$trace_dir_native" "${server_args[@]}"
}

test_stored_values() {
	# Split metadata, where the new metadata requires additional stored
	# value slots in CTF message iterators.
	local test_text="split metadata requiring additional stored values"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/stored-values -c sink.text.details"
	local server_args=("$test_data_dir/stored-values.json")
	local expected_stdout="${test_data_dir}/stored-values.expect"
	local expected_stderr="/dev/null"
	local tmp_dir

	tmp_dir=$(mktemp -d -t 'test-stored-value.XXXXXXX')

	# Generate test trace.
	bt_gen_mctf_trace "${trace_dir}/live/stored-values.mctf" "$tmp_dir/stored-values"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$tmp_dir" "${server_args[@]}"

	rm -rf "$tmp_dir"
}

test_live_new_stream_during_inactivity() {
	# Announce a new stream while an existing stream is inactive.
	# This requires the live consumer to check for new announced streams
	# when it receives inactivity beacons.
	local test_text="new stream announced while an existing stream is inactive"
	local cli_args_template="-i lttng-live net://localhost:@PORT@/host/hostname/new-streams -c sink.text.details"
	local server_args=("$test_data_dir/new-streams.json")
	local expected_stdout="${test_data_dir}/new-streams.expect"
	local expected_stderr="/dev/null"
	local tmp_dir

	tmp_dir=$(mktemp -d -t 'test-new-streams.XXXXXXX')

	# Generate test trace.
	bt_gen_mctf_trace "${trace_dir}/live/new-streams/first-trace.mctf" "$tmp_dir/first-trace"
	bt_gen_mctf_trace "${trace_dir}/live/new-streams/second-trace.mctf" "$tmp_dir/second-trace"

	run_test "$test_text" "$cli_args_template" "$expected_stdout" \
		"$expected_stderr" "$tmp_dir" "${server_args[@]}"

	rm -rf "$tmp_dir"
}

plan_tests 20

test_list_sessions
test_base
test_multi_domains
test_rate_limited
test_compare_to_ctf_fs
test_inactivity_discarded_packet
test_split_metadata
test_stored_values
test_live_new_stream_during_inactivity
