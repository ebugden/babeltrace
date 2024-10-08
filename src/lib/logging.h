/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2017-2018 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_LIB_LOGGING_H
#define BABELTRACE_LIB_LOGGING_H

#include <stdarg.h>

#ifndef BT_LOG_TAG
# error Please define a tag with BT_LOG_TAG before including this file.
#endif

#define BT_LOG_OUTPUT_LEVEL bt_lib_log_level

#include "logging/log.h"

#define BT_LIB_LOG_LIBBABELTRACE2_NAME "libbabeltrace2"

extern
int bt_lib_log_level;

#define BT_LIB_LOG(_lvl, _fmt, ...)					\
	do {								\
		if (BT_LOG_ON(_lvl)) {					\
			bt_lib_log(__FILE__, __func__,			\
				__LINE__, _lvl, _BT_LOG_TAG,		\
				(_fmt), ##__VA_ARGS__);			\
		}							\
	} while (0)

/* See `CONTRIBUTING.adoc` for usage */
#define BT_LIB_LOGF(_fmt, ...)	BT_LIB_LOG(BT_LOG_FATAL, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGE(_fmt, ...)	BT_LIB_LOG(BT_LOG_ERROR, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGW(_fmt, ...)	BT_LIB_LOG(BT_LOG_WARNING, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGI(_fmt, ...)	BT_LIB_LOG(BT_LOG_INFO, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGD(_fmt, ...)	BT_LIB_LOG(BT_LOG_DEBUG, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGT(_fmt, ...)	BT_LIB_LOG(BT_LOG_TRACE, _fmt, ##__VA_ARGS__)

/*
 * Log statement, specialized for the Babeltrace library.
 *
 * This function does NOT check that logging is enabled for level `lvl`:
 * you must check it manually with BT_LOG_ON().
 *
 * Use one of the BT_LIB_LOG*() macros above instead of calling this
 * function directly.
 */
void bt_lib_log(const char *file, const char *func, unsigned line,
		int lvl, const char *tag, const char *fmt, ...);

void bt_lib_log_v(const char *file, const char *func, unsigned line,
		int lvl, const char *tag, const char *fmt, va_list *args);

#define BT_LIB_LOG_AND_APPEND(_lvl, _fmt, ...)				\
	do {								\
		bt_lib_maybe_log_and_append_cause(			\
			__func__, __FILE__,				\
			__LINE__, _lvl, _BT_LOG_TAG,			\
			(_fmt), ##__VA_ARGS__);				\
	} while (0)

/* See `CONTRIBUTING.adoc` for usage */
#define BT_LIB_LOGE_APPEND_CAUSE(_fmt, ...)				\
	BT_LIB_LOG_AND_APPEND(BT_LOG_ERROR, _fmt, ##__VA_ARGS__)
#define BT_LIB_LOGW_APPEND_CAUSE(_fmt, ...)				\
	BT_LIB_LOG_AND_APPEND(BT_LOG_WARNING, _fmt, ##__VA_ARGS__)

/*
 * Like bt_lib_log(), but also appends a cause to the current thread's
 * error object.
 *
 * Note that, unlike bt_lib_log(), this function does check that logging
 * is enabled for level `lvl` before logging. This is to ensure that,
 * even though logging is disabled, the function still appends an error
 * cause, as the error reporting system does not rely on logging.
 *
 * Use one of the BT_LIB_LOG*_APPEND_CAUSE() macros above instead of
 * calling this function directly.
 */
void bt_lib_maybe_log_and_append_cause(const char *func, const char *file,
		unsigned line, int lvl, const char *tag,
		const char *fmt, ...);

#define BT_LIB_LOG_SUPPORTED

#endif /* BABELTRACE_LIB_LOGGING_H */
