/* util.c
 *
 * Copyright (C) 2025 Timo Kokkonen
 * All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of JPEGoptim.
 *
 * JPEGoptim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JPEGoptim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JPEGoptim. If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "littlefs-toy.h"

#define BUF_SIZE (64 * 1024)



int file_set_zero(int fd, off_t offset, off_t size)
{
	if (fd < 0)
		return -1;

	if (size > 0) {
		off_t written = 0;
		off_t curr_pos;
		void *buf;

		if ((curr_pos = lseek(fd, 0, SEEK_CUR)) < 0)
			return -2;

		if (!(buf = calloc(1, BUF_SIZE)))
			return -3;

		while (written < size) {
			size_t len = ((size - written) > BUF_SIZE ? BUF_SIZE : (size - written));
			if (write(fd, buf, len) < len)
				break;
			written += len;
		}
		free(buf);

		if (written < size)
			return -4;

		if (lseek(fd, curr_pos, SEEK_SET) < 0)
			return -5;
	}

	return 0;
}


int create_file(const char *name, off_t size)
{
        int fd, res;

        if (!name)
                return -1;

#ifdef WIN32
        fd = open(name, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, _S_IREAD | _S_IWRITE);
#else
        fd = open(name, O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
#endif
	if (fd < 0)
		warn("failed to create file: %s (%d)", name, errno);
	else if (size > 0) {
		if ((res = file_set_zero(fd, 0, size)))
			warn("failed to create empty file: %s (%d)", name, res);
	}

        return fd;
}


int open_file(const char *name, bool readonly)
{
	int fd;

	if (!name)
		return -1;

#ifdef WIN32
        fd = open(name, (readonly ? O_RDONLY : O_RDWR) | O_BINARY);
#else
        fd = open(name, (readonly ? O_RDONLY : O_RDWR));
#endif

	return fd;
}


int read_file(int fd, void *buf, size_t size)
{
	size_t bytes_read = 0;
	ssize_t len;

	do {
		len = read(fd, buf + bytes_read, (size - bytes_read));
		if (len > 0)
			bytes_read += len;
	} while (len > 0 && bytes_read < size);

	return (bytes_read == size ? 0 : -1);
}


int write_file(int fd, void *buf, size_t size)
{
	size_t bytes_written = 0;
	ssize_t len;

	do {
		len = write(fd, buf + bytes_written, (size - bytes_written));
		if (len > 0)
			bytes_written += len;
	} while (len > 0 && bytes_written < size);

	return (bytes_written == size ? 0 : -1);
}


off_t file_size(int fd)
{
	struct stat buf;

	if (fd < 0)
		return -1;
	if (fstat(fd, &buf) != 0)
		return -2;

	return buf.st_size;
}


int is_directory(const char *pathname)
{
	struct stat buf;

	if (!pathname)
		return 0;

	if (stat(pathname,&buf) != 0)
		return 0;

	return (S_ISDIR(buf.st_mode) ? 1 : 0);
}


int is_file(const char *filename, struct stat *st)
{
	struct stat buf;

	if (!filename)
		return 0;

	if (lstat(filename,&buf) != 0)
		return 0;
	if (st)
		*st=buf;

	return (S_ISREG(buf.st_mode) ? 1 : 0);
}


int file_exists(const char *pathname)
{
	struct stat buf;

	if (!pathname)
		return 0;

	return (stat(pathname,&buf) == 0 ? 1 : 0);
}



void fatal(const char *format, ...)
{
	va_list args;

	fprintf(stderr, PROGRAMNAME ": ");
	va_start(args,format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr,"\n");
	fflush(stderr);

	exit(1);
}


static bool warn_enabled = true;
static char last_warn[1024] = { 0 };

void warn(const char *format, ...)
{
	va_list args;

	if (warn_enabled) {
		fprintf(stderr, PROGRAMNAME ": ");
	}

	va_start(args,format);
	vsnprintf(last_warn, sizeof(last_warn), format, args);
	last_warn[sizeof(last_warn) - 1] = 0;
	va_end(args);

	if (warn_enabled) {
		fprintf(stderr,"%s\n", last_warn);
		fflush(stderr);
	}
}

const char* last_warning()
{
	return last_warn;
}

void warn_mode(bool enable)
{
	warn_enabled = enable;
}

/* eof :-) */
