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



/* util.c */
FILE* create_file(const char *name);
long file_size(FILE *fp);
int is_directory(const char *path);
int is_file(const char *filename, struct stat *st);
int file_exists(const char *pathname);
int rename_file(const char *old_path, const char *new_path);
int copy_file(const char *srcname, const char *dstname);
void fatal(const char *format, ...);
void warn(const char *format, ...);


#endif /* LITTLEFS_TOY_H */
