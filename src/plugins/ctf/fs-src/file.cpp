/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Philippe Proulx <pproulx@efficios.com>
 */

#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "cpp-common/vendor/fmt/format.h"

#include "file.hpp"

void ctf_fs_file_destroy(struct ctf_fs_file *file)
{
    if (!file) {
        return;
    }

    if (file->path) {
        g_string_free(file->path, TRUE);
    }

    delete file;
}

void ctf_fs_file_deleter::operator()(ctf_fs_file * const file) noexcept
{
    ctf_fs_file_destroy(file);
}

ctf_fs_file::UP ctf_fs_file_create(const bt2c::Logger& parentLogger)
{
    ctf_fs_file::UP file {new ctf_fs_file {parentLogger}};

    file->path = g_string_new(NULL);
    if (!file->path) {
        goto error;
    }

    goto end;

error:
    file.reset();

end:
    return file;
}

int ctf_fs_file_open(struct ctf_fs_file *file, const char *mode)
{
    int ret = 0;
    struct stat stat;

    BT_CPPLOGI_SPEC(file->logger, "Opening file \"{}\" with mode \"{}\"", file->path->str, mode);
    file->fp.reset(fopen(file->path->str, mode));
    if (!file->fp) {
        BT_CPPLOGE_ERRNO_APPEND_CAUSE_SPEC(file->logger, "Cannot open file", ": path={}, mode={}",
                                           file->path->str, mode);
        goto error;
    }

    BT_CPPLOGI_SPEC(file->logger, "Opened file: {}", fmt::ptr(file->fp));

    if (fstat(fileno(file->fp.get()), &stat)) {
        BT_CPPLOGE_ERRNO_APPEND_CAUSE_SPEC(file->logger, "Cannot get file information", ": path={}",
                                           file->path->str);
        goto error;
    }

    file->size = stat.st_size;
    BT_CPPLOGI_SPEC(file->logger, "File is {} bytes", (intmax_t) file->size);
    goto end;

error:
    ret = -1;

end:
    return ret;
}
