#!/bin/bash
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2017 Philippe Proulx <pproulx@efficios.com>
#

SH_TAP=1

if [ -n "${BT_TESTS_SRCDIR:-}" ]; then
	UTILSSH="$BT_TESTS_SRCDIR/utils/utils.sh"
else
	UTILSSH="$(dirname "$0")/../../utils/utils.sh"
fi

# shellcheck source=../../utils/utils.sh
source "$UTILSSH"

tmp_stdout=$(mktemp -t test-convert-args-stdout.XXXXXX)
tmp_stderr=$(mktemp -t test-convert-args-stderr.XXXXXX)

test_bt_convert_run_args() {
	local what="$1"
	local convert_args="$2"
	local expected_run_args="$3"

	local run_args

	# Split argument string into array.
	IFS=' ' read -ra convert_args_array <<< "$convert_args"

	# Execute convert command.
	bt_cli "${tmp_stdout}" "${tmp_stderr}" convert --run-args "${convert_args_array[@]}"
	ok $? "${what}: success exit status"

	run_args=$(cat "${tmp_stdout}")

	# Verify output run args.
	[ "$run_args" = "$expected_run_args" ]
	ok $? "${what}: run arguments"
}

test_bt_convert_fails() {
	local what="$1"
	local convert_args="$2"
	local expected_error_str="$3"

	# Split argument string into array.
	IFS=' ' read -ra convert_args_array <<< "$convert_args"

	# Execute convert command.
	bt_cli "${tmp_stdout}" "${tmp_stderr}" convert --run-args "${convert_args_array[@]}"
	isnt "$?" 0 "failure exit status"

	# Nothing should be printed on stdout.
	bt_diff /dev/null "${tmp_stdout}"
	ok $? "$what: nothing is printed on stdout"

	# Check for expected error string in stderr.
	bt_grep --quiet --fixed-strings -e "$expected_error_str" "$tmp_stderr"
	local status=$?
	ok "$status" "$what: expected error message"
	if [ "$status" -ne 0 ]; then
		diag "Expected error string '${expected_error_str}' not found in stderr:"
		diag "$(cat "${tmp_stderr}")"
	fi
}

path_to_trace="${BT_CTF_TRACES_PATH}/1/succeed/succeed1"
path_to_trace2="${BT_CTF_TRACES_PATH}/1/succeed/succeed2"
output_path="/output/path"

if [ "$BT_TESTS_OS_TYPE" = "mingw" ]; then
	# Use Windows native paths for comparison because Unix
	# paths are converted by the shell before they are passed
	# to the native babeltrace2 binary.
	path_to_trace=$(cygpath -m "$path_to_trace")
	path_to_trace2=$(cygpath -m "$path_to_trace2")
	output_path=$(cygpath -m "$output_path")
fi

plan_tests 161

test_bt_convert_run_args 'path non-option arg' "$path_to_trace" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option args' "$path_to_trace $path_to_trace2" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\", \"${path_to_trace2}\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + named user source with --params' "$path_to_trace --component ZZ:source.another.source --params salut=yes" "--component ZZ:source.another.source --params salut=yes --component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect ZZ:muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'unnamed user source' '--component source.salut.com' "--component source.salut.com:source.salut.com --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect 'source\.salut\.com:muxer' --connect muxer:pretty"
test_bt_convert_run_args "path non-option arg + user source named \`auto-disc-source-ctf-fs\`" "--component auto-disc-source-ctf-fs:source.salut.com $path_to_trace" "--component auto-disc-source-ctf-fs:source.salut.com --component auto-disc-source-ctf-fs-0:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect auto-disc-source-ctf-fs-0:muxer --connect muxer:pretty"
test_bt_convert_run_args "path non-option arg + user sink named \`pretty\`" "--component pretty:sink.my.sink $path_to_trace" "--component pretty:sink.my.sink --component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args "path non-option arg + user filter named \`muxer\`" "--component muxer:filter.salut.com $path_to_trace" "--component muxer:filter.salut.com --component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer-0:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer-0 --connect muxer-0:muxer --connect muxer:pretty"
test_bt_convert_run_args "path non-option arg + --begin + user filter named \`trimmer\`" "$path_to_trace --component trimmer:filter.salut.com --begin=abc" "--component trimmer:filter.salut.com --component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component trimmer-0:filter.utils.trimmer --params 'begin=\"abc\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:trimmer-0 --connect trimmer-0:trimmer --connect trimmer:pretty"
test_bt_convert_run_args 'path non-option arg + --begin' "$path_to_trace --begin=123" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component trimmer:filter.utils.trimmer --params 'begin=\"123\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:trimmer --connect trimmer:pretty"
test_bt_convert_run_args 'path non-option arg + --begin --end' "$path_to_trace --end=456 --begin 123" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component trimmer:filter.utils.trimmer --params 'end=\"456\"' --params 'begin=\"123\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:trimmer --connect trimmer:pretty"
test_bt_convert_run_args 'path non-option arg + --timerange' "$path_to_trace --timerange=[abc,xyz]" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component trimmer:filter.utils.trimmer --params 'begin=\"abc\"' --params 'end=\"xyz\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:trimmer --connect trimmer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-cycles' "$path_to_trace --clock-cycles" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params clock-cycles=yes --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-date' "$path_to_trace --clock-date" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params clock-date=yes --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-force-correlate' "$path_to_trace --clock-force-correlate" "--component auto-disc-source-ctf-fs:source.ctf.fs --params force-clock-class-origin-unix-epoch=yes --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-gmt' "$path_to_trace --clock-gmt" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params clock-gmt=yes --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-offset' "$path_to_trace --clock-offset=15487" "--component auto-disc-source-ctf-fs:source.ctf.fs --params clock-class-offset-s=15487 --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-offset-ns' "$path_to_trace --clock-offset-ns=326159487" "--component auto-disc-source-ctf-fs:source.ctf.fs --params clock-class-offset-ns=326159487 --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --clock-seconds' "$path_to_trace --clock-seconds" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params clock-seconds=yes --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --color' "$path_to_trace --color=never" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params 'color=\"never\"' --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --debug-info' "$path_to_trace --debug-info" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component debug-info:filter.lttng-utils.debug-info --connect auto-disc-source-ctf-fs:muxer --connect muxer:debug-info --connect debug-info:pretty"
test_bt_convert_run_args 'path non-option arg + --debug-info-dir' "$path_to_trace --debug-info-dir=${output_path}" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component debug-info:filter.lttng-utils.debug-info --params 'debug-info-dir=\"${output_path}\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:debug-info --connect debug-info:pretty"
test_bt_convert_run_args 'path non-option arg + --debug-info-target-prefix' "$path_to_trace --debug-info-target-prefix=${output_path}" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component debug-info:filter.lttng-utils.debug-info --params 'target-prefix=\"${output_path}\"' --connect auto-disc-source-ctf-fs:muxer --connect muxer:debug-info --connect debug-info:pretty"
test_bt_convert_run_args 'path non-option arg + --debug-info-full-path' "$path_to_trace --debug-info-full-path" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --component debug-info:filter.lttng-utils.debug-info --params full-path=yes --connect auto-disc-source-ctf-fs:muxer --connect muxer:debug-info --connect debug-info:pretty"
test_bt_convert_run_args 'path non-option arg + --fields=trace:domain,loglevel' "--fields=trace:domain,loglevel $path_to_trace" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params field-trace:domain=yes,field-loglevel=yes,field-default=hide --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --fields=all' "--fields=all $path_to_trace" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params field-default=show --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --names=context,header' "--names=context,header $path_to_trace" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params name-context=yes,name-header=yes,name-default=hide --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --names=all' "--names=all $path_to_trace" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params name-default=show --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --no-delta' "$path_to_trace --no-delta" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params no-delta=yes --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + --output' "$path_to_trace --output $output_path" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --params 'path=\"$output_path\"' --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + -i ctf' "$path_to_trace -i ctf" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:pretty"
test_bt_convert_run_args 'URL non-option arg + -i lttng-live' 'net://some-host/host/target/session -i lttng-live' "--component lttng-live:source.ctf.lttng-live --params 'inputs=[\"net://some-host/host/target/session\"]' --params 'session-not-found-action=\"end\"' --component pretty:sink.text.pretty --component muxer:filter.utils.muxer --connect lttng-live:muxer --connect muxer:pretty"
test_bt_convert_run_args 'path non-option arg + -o dummy' "$path_to_trace -o dummy" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component dummy:sink.utils.dummy --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:dummy"
test_bt_convert_run_args 'path non-option arg + -o ctf + --output' "$path_to_trace -o ctf --output $output_path" "--component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component sink-ctf-fs:sink.ctf.fs --params 'path=\"$output_path\"' --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect muxer:sink-ctf-fs"
test_bt_convert_run_args 'path non-option arg + user sink with log level' "$path_to_trace -c sink.mein.sink -lW" "--component sink.mein.sink:sink.mein.sink --log-level W --component auto-disc-source-ctf-fs:source.ctf.fs --params 'inputs=[\"$path_to_trace\"]' --component muxer:filter.utils.muxer --connect auto-disc-source-ctf-fs:muxer --connect 'muxer:sink\.mein\.sink'"

test_bt_convert_fails \
	'bad --component format (plugin only)' \
	'--component salut' \
	"Invalid format for --component option's argument:"

test_bt_convert_fails \
	'bad --component format (name and plugin only)' \
	'--component name:salut' \
	"Missing component class type (\`source\`, \`filter\`, or \`sink\`)."

test_bt_convert_fails \
	'bad --component format (name only)' \
	'--component name:' \
	"Missing component class type (\`source\`, \`filter\`, or \`sink\`)."

test_bt_convert_fails \
	'bad --component format (extra dot found)' \
	'--component name:source.plugin.comp.cls' \
	"Invalid format for --component option's argument:"

test_bt_convert_fails \
	'duplicate component name' \
	'--component hello:sink.a.b --component hello:source.c.d' \
	'Duplicate component instance name:'

test_bt_convert_fails \
	'unknown option' \
	'--component hello:sink.a.b --salut' \
	"Unknown option \`--salut\`"

# The error string spans two lines in this error message, it's not convenient to
# check for multiple lines, so we just check the first line.
test_bt_convert_fails \
	'--params without current component' \
	'--params lol=23' \
	"No current component (--component option) or non-option argument of which to"

test_bt_convert_fails \
	'duplicate --begin' \
	'--begin abc --clock-seconds --begin cde' \
	'At --begin option: --begin or --timerange option already specified'

test_bt_convert_fails \
	'duplicate --end' \
	'--begin abc --end xyz --clock-seconds --end cde' \
	'At --end option: --end or --timerange option already specified'

test_bt_convert_fails \
	'--begin and --timerange' \
	'--begin abc --clock-seconds --timerange abc,def' \
	'At --timerange option: --begin, --end, or --timerange option already specified'

test_bt_convert_fails \
	'--end and --timerange' \
	'--end abc --clock-seconds --timerange abc,def' \
	'At --timerange option: --begin, --end, or --timerange option already specified'

test_bt_convert_fails \
	'bad --timerange format (1)' \
	'--timerange abc' \
	"Invalid --timerange option's argument: expecting BEGIN,END or [BEGIN,END]:"

test_bt_convert_fails \
	'bad --timerange format (2)' \
	'--timerange abc,' \
	"Invalid --timerange option's argument: expecting BEGIN,END or [BEGIN,END]:"

test_bt_convert_fails \
	'bad --timerange format (3)' \
	'--timerange ,cde' \
	"Invalid --timerange option's argument: expecting BEGIN,END or [BEGIN,END]:"

test_bt_convert_fails \
	'bad --fields format' \
	'--fields salut' \
	"Unknown field: \`salut\`."

test_bt_convert_fails \
	'bad --names format' \
	'--names salut' \
	"Unknown name: \`salut\`."

test_bt_convert_fails \
	'unknown -i' \
	'-i lol' \
	'Unknown legacy input format:'

test_bt_convert_fails \
	'duplicate -i' \
	'-i lttng-live --clock-seconds --input-format=ctf' \
	'Duplicate --input-format option.'

test_bt_convert_fails \
	'unknown -o' \
	'-o lol' \
	'Unknown legacy output format:'

test_bt_convert_fails \
	'duplicate -o' \
	'-o dummy --clock-seconds --output-format=text' \
	'Duplicate --output-format option.'

test_bt_convert_fails \
	'--run-args and --run-args-0' \
	"$path_to_trace --run-args --run-args-0" \
	'Cannot specify --run-args and --run-args-0.'

test_bt_convert_fails \
	'-o ctf-metadata without path' \
	'-o ctf-metadata' \
	'--output-format=ctf-metadata specified without a path.'

test_bt_convert_fails \
	'-i lttng-live and implicit source.ctf.fs' \
	'-i lttng-live net://some-host/host/target/session --clock-offset=23' \
	'--clock-offset specified, but no source.ctf.fs component instantiated.'

test_bt_convert_fails \
	'implicit source.ctf.fs without path' \
	'--clock-offset=23' \
	'--clock-offset specified, but no source.ctf.fs component instantiated.'

test_bt_convert_fails \
	'implicit source.ctf.lttng-live without URL' \
	'-i lttng-live' \
	"Missing URL for implicit \`source.ctf.lttng-live\` component."

test_bt_convert_fails \
	'no source' \
	'-o text' \
	'No source component.'

test_bt_convert_fails \
	'-o ctf without --output' \
	'my-trace -o ctf' \
	'--output-format=ctf specified without --output (trace output path).'

# The error string spans two lines in this error message, it's not convenient to
# check for multiple lines, so we just check the first line.
test_bt_convert_fails \
	'-o ctf + --output with implicit sink.text.pretty' \
	"my-trace -o ctf --output $output_path --no-delta" \
	'Ambiguous --output option: --output-format=ctf specified but another option'

test_bt_convert_fails \
	'--stream-intersection' \
	"$path_to_trace --stream-intersection" \
	'Cannot specify --stream-intersection with --run-args or --run-args-0.'

test_bt_convert_fails \
	'two sinks with -o dummy + --clock-seconds' \
	"$path_to_trace -o dummy --clock-seconds" \
	'More than one sink component specified.'

test_bt_convert_fails \
	'path non-option arg + user sink + -o text' \
	"$path_to_trace --component=sink.abc.def -o text" \
	'More than one sink component specified.'

rm -f "${tmp_stdout}"
rm -f "${tmp_stderr}"
