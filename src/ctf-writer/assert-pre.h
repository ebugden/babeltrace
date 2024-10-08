/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2018 EfficiOS Inc. and Linux Foundation
 * Copyright (c) 2018 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_CTF_WRITER_ASSERT_PRE_H
#define BABELTRACE_CTF_WRITER_ASSERT_PRE_H

/*
 * The macros in this header use macros defined in "logging/log.h". We
 * don't want this header to automatically include "logging/log.h"
 * because you need to manually define BT_LOG_TAG before including
 * "logging/log.h" and it is unexpected that you also need to define it
 * before including this header.
 *
 * This is a reminder that in order to use "ctf-writer/assert-pre.h",
 * you also need to use logging explicitly.
 */

#ifndef BT_LOG_SUPPORTED
# error Include "logging/log.h" before this header.
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include "common/macros.h"

#ifdef BT_DEV_MODE
/*
 * Prints the details of an unsatisfied precondition without immediately
 * aborting. You should use this within a function which checks
 * preconditions, but which is called from a BT_CTF_ASSERT_PRE()
 * context, so that the function can still return its result for
 * BT_CTF_ASSERT_PRE() to evaluate it.
 *
 * Example:
 *
 *     BT_CTF_ASSERT_PRE_FUNC
 *     static inline bool check_complex_precond(...)
 *     {
 *         ...
 *
 *         if (...) {
 *             BT_CTF_ASSERT_PRE_MSG("Invalid object: ...", ...);
 *             return false;
 *         }
 *
 *         ...
 *     }
 *
 *     ...
 *
 *     BT_CTF_ASSERT_PRE(check_complex_precond(...),
 *                   "Precondition is not satisfied: ...", ...);
 */
# define BT_CTF_ASSERT_PRE_MSG(_fmt, ...)				\
	do {								\
		bt_log_write_printf(__FILE__, __func__,			\
			__LINE__, BT_LOG_FATAL, BT_LOG_TAG, (_fmt),	\
			##__VA_ARGS__);					\
	} while (0)

/*
 * Asserts that the library precondition _cond is satisfied.
 *
 * If _cond is false, log a fatal statement using _fmt and the optional
 * arguments using BT_LOGF(), and abort.
 *
 * To assert that a postcondition is satisfied or that some internal
 * object/context/value is in the expected state, use BT_ASSERT_DBG().
 */
# define BT_CTF_ASSERT_PRE(_cond, _fmt, ...)				\
	do {								\
		if (!(_cond)) {						\
			BT_CTF_ASSERT_PRE_MSG("CTF writer precondition not satisfied; error is:"); \
			BT_CTF_ASSERT_PRE_MSG((_fmt), ##__VA_ARGS__);	\
			BT_CTF_ASSERT_PRE_MSG("Aborting...");		\
			bt_common_abort();					\
		}							\
	} while (0)

/*
 * Marks a function as being only used within a BT_CTF_ASSERT_PRE() context.
 */
# define BT_CTF_ASSERT_PRE_FUNC

/*
 * Prints the details of an unsatisfied precondition without immediately
 * aborting. You should use this within a function which checks
 * preconditions, but which is called from a BT_CTF_ASSERT_PRE() context, so
 * that the function can still return its result for BT_CTF_ASSERT_PRE() to
 * evaluate it.
 *
 * Example:
 *
 *     BT_CTF_ASSERT_PRE_FUNC
 *     static inline bool check_complex_precond(...)
 *     {
 *         ...
 *
 *         if (...) {
 *             BT_CTF_ASSERT_PRE_MSG("Invalid object: ...", ...);
 *             return false;
 *         }
 *
 *         ...
 *     }
 *
 *     ...
 *
 *     BT_CTF_ASSERT_PRE(check_complex_precond(...),
 *                   "Precondition is not satisfied: ...", ...);
 */
#else
# define BT_CTF_ASSERT_PRE(_cond, _fmt, ...)	((void) sizeof((void) (_cond), 0))
# define BT_CTF_ASSERT_PRE_FUNC	__attribute__((unused))
# define BT_CTF_ASSERT_PRE_MSG(_fmt, ...)
#endif /* BT_DEV_MODE */

/*
 * Developer mode: asserts that a given variable is not NULL.
 */
#define BT_CTF_ASSERT_PRE_NON_NULL(_obj, _obj_name)				\
	BT_CTF_ASSERT_PRE((_obj), "%s is NULL: ", _obj_name)

/*
 * Developer mode: asserts that a given object is NOT frozen. This macro
 * checks the `frozen` field of _obj.
 */
#define BT_CTF_ASSERT_PRE_HOT(_obj, _obj_name, _fmt, ...)			\
	BT_CTF_ASSERT_PRE(!(_obj)->frozen, "%s is frozen" _fmt, _obj_name,	\
		##__VA_ARGS__)

/*
 * Developer mode: asserts that a given index is less than a given size.
 */
#define BT_CTF_ASSERT_PRE_VALID_INDEX(_index, _length)			\
	BT_CTF_ASSERT_PRE((_index) < (_length),				\
		"Index is out of bounds: index=%" PRIu64 ", "		\
		"count=%" PRIu64, (uint64_t) (_index), (uint64_t) (_length))

#endif /* BABELTRACE_CTF_WRITER_ASSERT_PRE_H */
