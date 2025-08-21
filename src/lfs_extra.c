/* lfs_extra.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <lfs.h>
#include <lfs_util.h>

#include "lfs_extra.h"


int lfs_mkdir_parent(lfs_t *lfs, const char *pathname)
{
	struct lfs_info info;
	char *path, *tok, *saveptr;
	int res = LFS_ERR_OK;


	if (!lfs || !pathname)
		return LFS_ERR_INVAL;

	/* Check if directory already exists */
	if (lfs_stat(lfs, pathname, &info) == LFS_ERR_OK)
		return LFS_ERR_OK;

	if (!(path = strdup(pathname)))
		return LFS_ERR_NOMEM;

	if ((tok = strtok_r(path, "/", &saveptr))) {
		while (tok) {
			res = lfs_mkdir(lfs, path);
			if (res != LFS_ERR_EXIST && res != LFS_ERR_OK)
				break;
			*(saveptr - 1) = '/';
			tok = strtok_r(NULL, "/", &saveptr);
		}
	} else {
		res = LFS_ERR_NOENT;
	}

	free(path);

	return res;
}


int lfs_rmdir_recursive(lfs_t *lfs, const char *pathname)
{
	struct lfs_info st;
	lfs_dir_t dir;
	char separator[2] = "/";
	char fullname[LFS_NAME_MAX * 2];
	size_t path_len;
	int res = LFS_ERR_OK;


	if (lfs_stat(lfs, pathname, &st) != LFS_ERR_OK)
		return LFS_ERR_NOENT;
	if (st.type != LFS_TYPE_DIR)
		return LFS_ERR_NOTDIR;

	/* Check if path ends with "/" ... */
	path_len = strnlen(pathname, LFS_NAME_MAX);
	if (path_len > 0) {
		if (pathname[path_len - 1] == '/')
			separator[0] = 0;
	}

	/* Read and delete entries in the directory */
	if ((res = lfs_dir_open(lfs, &dir, pathname)) != LFS_ERR_OK)
		return res;
	while (lfs_dir_read(lfs, &dir, &st) > 0) {
		/* Skip special directories ("." and "..") */
		if (st.name[0] == '.') {
			if (st.name[1] == 0)
				continue;
			if (st.name[1] == '.' && st.name[2] == 0)
				continue;
		}

		snprintf(fullname, sizeof(fullname), "%s%s%s", pathname, separator, st.name);
		fullname[LFS_NAME_MAX] = 0;

		if (st.type == LFS_TYPE_DIR) {
			if ((res = lfs_rmdir_recursive(lfs, fullname)) != LFS_ERR_OK)
				break;
		} else {
			if ((res = lfs_remove(lfs, fullname)) != LFS_ERR_OK)
				break;
		}
	}
	lfs_dir_close(lfs, &dir);

	if (res == LFS_ERR_OK) {
		res = lfs_remove(lfs, pathname);
	}

	return res;
}


