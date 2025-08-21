/* littlefs-toy.h
   Copyright (C) 2025 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of LittleFS-Toy.

   LittleFS-Toy is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   LittleFS-Toy is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with LittleFS-Toy. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LITTLEFS_TOY_H
#define LITTLEFS_TOY_H

#include <sys/stat.h>
#include <stdbool.h>
#include <stdint.h>
#include "config.h"



enum lfs_commands {
	LFS_NONE   = 0,
	LFS_LIST   = 1,
	LFS_CREATE = 2,
	LFS_UPDATE = 3,
	LFS_DELETE = 4,
};

typedef struct param_t {
	const char *name;
	bool found;
	struct param_t *next;
} param_t;


/* util.c */
int create_file(const char *name, off_t size);
int open_file(const char *name, bool readonly);
int file_set_zero(int fd, off_t offset, off_t size);
int read_file(int fd, off_t offset, void *buf, size_t size);
int write_file(int fd, off_t offset, void *buf, size_t size);
off_t file_size(int fd);
int is_directory(const char *path);
int is_file(const char *filename, struct stat *st);
int file_exists(const char *pathname);
int rename_file(const char *old_path, const char *new_path);
char *trim_str(char *s);
int parse_int_str(const char *str, int64_t *val, int64_t min, int64_t max);
void fatal(const char *format, ...);
void warn(const char *format, ...);
const char* warn_last_msg();
void warn_clear_last_msg();
void warn_mode(bool enabled);

#endif /* LITTLEFS_TOY_H */
