/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2019 Michael Jeanson <mjeanson@efficios.com>
 */

#ifndef BABELTRACE_COMMON_UUID_H
#define BABELTRACE_COMMON_UUID_H

#include <inttypes.h>
#include <stdint.h>
#include "common/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_UUID_STR_LEN 36 /* Excludes final \0 */
#define BT_UUID_LEN 16
#define BT_UUID_VER 4

#define BT_UUID_FMT \
	"%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "-%02" SCNx8 \
	"%02" SCNx8 "-%02" SCNx8 "%02" SCNx8 "-%02" SCNx8 "%02" SCNx8 \
	"-%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 "%02" SCNx8 \
	"%02" SCNx8

#define BT_UUID_FMT_VALUES(uuid) \
	(uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], \
	(uuid)[6], (uuid)[7], (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], \
	(uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define BT_UUID_SCAN_VALUES(uuid) \
	&(uuid)[0], &(uuid)[1], &(uuid)[2], &(uuid)[3], &(uuid)[4], &(uuid)[5], \
	&(uuid)[6], &(uuid)[7], &(uuid)[8], &(uuid)[9], &(uuid)[10], &(uuid)[11], \
	&(uuid)[12], &(uuid)[13], &(uuid)[14], &(uuid)[15]

typedef uint8_t bt_uuid_t[BT_UUID_LEN];

void bt_uuid_generate(bt_uuid_t uuid_out);
void bt_uuid_to_str(const bt_uuid_t uuid_in, char *str_out);
int bt_uuid_from_c_str(const char *str, bt_uuid_t uuid_out);
int bt_uuid_from_str(const char *begin, const char *end, bt_uuid_t uuid_out);
int bt_uuid_compare(const bt_uuid_t uuid_a, const bt_uuid_t uuid_b);
void bt_uuid_copy(bt_uuid_t uuid_dest, const bt_uuid_t uuid_src);

#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE_COMMON_UUID_H */
