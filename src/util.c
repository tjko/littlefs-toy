/* util.c
 *
 * Copyright (C) 2025 Timo Kokkonen
 * All Rights Reserved.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of LittleFS-Toy.
 *
 * LittleFS-Toy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LittleFS-Toy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LittleFS-Toy. If not, see <https://www.gnu.org/licenses/>.
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
#include <ctype.h>
#include <wctype.h>
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
		if (lseek(fd, offset, SEEK_SET) < 0)
			return -3;
		if (!(buf = calloc(1, BUF_SIZE)))
			return -4;

		while (written < size) {
			ssize_t len = ((size - written) > BUF_SIZE ? BUF_SIZE : (size - written));
			ssize_t wrote;

			do {
				wrote = write(fd, buf, len);
			} while (wrote < 0 && errno == EINTR);
			if (wrote <= 0)
				break;
			written += len;
		}
		free(buf);

		if (written < size)
			return -5;

		if (lseek(fd, curr_pos, SEEK_SET) < 0)
			return -6;
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


int read_file(int fd, off_t offset, void *buf, size_t size)
{
	size_t bytes_read = 0;
	ssize_t len;


	if (offset >= 0) {
		if (lseek(fd, offset, SEEK_SET) < 0)
			return -2;
	}

	do {
		do {
			len = read(fd, buf + bytes_read, (size - bytes_read));
		} while (len < 0 && errno == EINTR);
		if (len > 0)
			bytes_read += len;
	} while (len > 0 && bytes_read < size);

	return (bytes_read == size ? 0 : -1);
}


int write_file(int fd, off_t offset, void *buf, size_t size)
{
	size_t bytes_written = 0;
	ssize_t len;


	if (offset >= 0) {
		if (lseek(fd, offset, SEEK_SET) < 0)
			return -2;
	}

	do {
		do {
			len = write(fd, buf + bytes_written, (size - bytes_written));
		} while (len < 0 && errno == EINTR);
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


int mkdir_parent(const char *pathname, mode_t mode)
{
	int res = 0;
	char *dir, *tok, *saveptr;

	if (!pathname || *pathname == 0)
		return -1;
	if (!(dir = strdup(pathname)))
		return -2;
	if (!(tok = strtok_r(dir, "/", &saveptr)))
		res = -3;

	if (res == 0) {
		while (tok) {
			if (dir[0] == '.' && dir[1] == '.' && dir[2] == 0) {
				res = -4;
				break;
			}
			if (!(dir[0] == '.' && dir[1] == 0)) {
				if (mkdir(dir, mode) < 0 && errno != EEXIST) {
					res = -5;
					break;
				}
			}
			if (saveptr)
				*(saveptr - 1) = '/';
			tok = strtok_r(NULL, "/", &saveptr);
		}
	}

	free(dir);

	return res;
}

char *trim_str(char *s)
{
	char *e;

	if (!s) return NULL;

	while (iswspace(*s))
		s++;
	e = s + strlen(s) - 1;
	while (e >= s && iswspace(*e))
		*e-- = 0;

	return s;
}


char *splitdir(const char *filename)
{
	char *buf = NULL;
	ssize_t len;
	char *s;

	if (!filename)
		return NULL;
	if (!(s = strrchr(filename, '/')))
		return NULL;

	/* Create new string that contains the directory portion */
	len = s - filename + 1;
	if (!(buf = calloc(1, len)))
		return NULL;
	if (len > 1)
		memcpy(buf, filename, len - 1);

	return buf;
}

int parse_int_str(const char *str, int64_t *val, int64_t min, int64_t max)
{
	char *str_copy = NULL;
	int64_t v = 0;
	int64_t m = 1;
	int base = 10;
	int res = 0;
	char *suffix, *s;
	size_t len;


	if (!str || !val)
		return -1;
	if (!(str_copy = strdup(str)))
		return -2;

	/* Trim any whitespace from beginning and end */
	s = trim_str(str_copy);
	if (!s || (len = strlen(s)) < 1) {
		res = -3;
	}

	/* Attempt to parse string */
	if (res == 0) {
		/* Check prefix (for hex) */
		if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
			base = 16;

		v = strtoll(s, &suffix, base);
		if (s == suffix)
			res = 1;
	}

	/* Check suffix */
	if (res == 0) {
		while (*suffix && isspace(*suffix))
			suffix++;
		if (*suffix) {
			switch (*suffix) {
			case 'K':
				m = 1024;
				break;
			case 'k':
				m = 1000;
				break;
			case 'M':
				m = 1048576;
				break;
			case 'm':
				m = 1000000;
				break;
			case 'G':
				m = 1073741824;
				break;
			case 'g':
				m = 1000000000;
				break;
			case 'T':
				m = 1099511627776L;
				break;
			case 't':
				m = 1000000000000L;
				break;

			default:
				res = -4;
			}
		}
	}

	/* Apply suffix (multiplier) */
	if (res == 0) {
		v *= m;
		if (v < min || v > max)
			res = 2;
	}


	if (res == 0)
		*val = v;
	if (str_copy)
		free(str_copy);

	return res;
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

const char* warn_last_msg()
{
	return last_warn;
}

void warn_clear_last_msg()
{
	last_warn[0] = 0;
}

void warn_mode(bool enable)
{
	warn_enabled = enable;
}

/* eof :-) */
