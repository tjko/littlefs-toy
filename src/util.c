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
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "littlefs-toy.h"

FILE* create_file(const char *name)
{
        FILE *f;
        int fd;

        if (!name)
                return NULL;

#ifdef WIN32
        fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, _S_IREAD | _S_IWRITE);
#else
        fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
#endif
        if (fd < 0)
                return NULL;
        if (!(f = fdopen(fd, "wb"))) {
                close(fd);
                return NULL;
        }

        return f;
}



long file_size(FILE *fp)
{
	struct stat buf;

	if (!fp)
		return -1;
	if (fstat(fileno(fp),&buf) != 0)
		return -2;

	return (long)buf.st_size;
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



#define COPY_BUF_SIZE  (256 * 1024)

int copy_file(const char *srcfile, const char *dstfile)
{
	FILE *in,*out;
	unsigned char *buf;
	int r,w;
	int err=0;

	if (!srcfile || !dstfile)
		return -1;

	if (!(in = fopen(srcfile, "rb"))) {
		warn("failed to open file for reading: %s", srcfile);
		return -2;
	}
	if (!(out = create_file(dstfile))) {
		fclose(in);
		warn("failed to open file for writing: %s", dstfile);
		return -3;
	}

	if (!(buf = calloc(COPY_BUF_SIZE, 1)))
		fatal("out of memory");


	do {
		r = fread(buf, 1, COPY_BUF_SIZE, in);
		if (r > 0) {
			w = fwrite(buf, 1, r, out);
			if (w != r) {
				err=1;
				warn("error writing to file: %s", dstfile);
				break;
			}
		} else {
			if (ferror(in)) {
				err=2;
				warn("error reading from file: %s", srcfile);
				break;
			}
		}
	} while (!feof(in));

	fclose(out);
	fclose(in);
	free(buf);

	return err;
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

	exit(3);
}


void warn(const char *format, ...)
{
	va_list args;


	fprintf(stderr, PROGRAMNAME ": ");
	va_start(args,format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr,"\n");
	fflush(stderr);
}


/* eof :-) */
