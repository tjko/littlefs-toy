/* lfs_extra.h
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

#ifndef _LFS_EXTRA_H_
#define _LFS_EXTRA_H_

#include <lfs.h>

#ifdef __cplusplus
extern "C"
{
#endif


int lfs_mkdir_parent(lfs_t *lfs, const char *pathname);
int lfs_rmdir_recursive(lfs_t *lfs, const char *pathname);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LFS_EXTRA_H_ */
