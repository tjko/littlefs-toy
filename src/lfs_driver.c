/* lfs_driver.c
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <pthread.h>
#include <lfs.h>
#include <lfs_util.h>

#include "lfs_driver.h"


static int block_device_read(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, void *buffer, lfs_size_t size)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;


	if (c->block_count > 0 && block >= c->block_count) {
		LFS_ERROR("attempt to read past end of filesystem");
		return LFS_ERR_IO;
	}
	if (off + size > c->block_size) {
		LFS_ERROR("attempt to read past end of block");
		return LFS_ERR_IO;
	}

	if (ctx->fd >= 0) {
		off_t f_offset = ctx->offset + (block * c->block_size) + off;
		if (lseek(ctx->fd, f_offset, SEEK_SET) < 0) {
			LFS_ERROR("seek failed: %ld (errno=%d)", f_offset, errno);
			return LFS_ERR_IO;
		}
		if (read(ctx->fd, buffer, size) < size) {
			LFS_ERROR("failed to read file");
			return LFS_ERR_IO;
		}
	} else {
		memcpy(buffer, ctx->base + (block * c->block_size) + off, size);
	}
	return LFS_ERR_OK;
}

static int block_device_prog(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, const void *buffer, lfs_size_t size)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	if (block >= c->block_count) {
		LFS_ERROR("attempt to write past end of filesystem");
		return LFS_ERR_IO;
	}
	if (off % c->prog_size != 0) {
		LFS_ERROR("flash address must be aligned to flash page");
		return LFS_ERR_IO;
	}
	if (size % c->prog_size != 0) {
		LFS_ERROR("bytes to write must be multiple of flash page size");
		return LFS_ERR_IO;
	}
	if (off + size > c->block_size) {
		LFS_ERROR("write must be within a block");
		return LFS_ERR_IO;
	}

	if (ctx->fd >= 0) {
		off_t f_offset = ctx->offset + (block * c->block_size) + off;
		if (lseek(ctx->fd, f_offset, SEEK_SET) < 0) {
			LFS_ERROR("seek failed: %ld", f_offset);
			return LFS_ERR_IO;
		}
		if (write(ctx->fd, buffer, size) < size) {
			LFS_ERROR("failed to read file");
			return LFS_ERR_IO;
		}
	} else {
		memcpy(ctx->base + (block * c->block_size) + off, buffer, size);
	}

	return LFS_ERR_OK;
}

static int block_device_erase(const struct lfs_config *, lfs_block_t)
{
	return LFS_ERR_OK;
}

static int block_device_sync(const struct lfs_config *c)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	if (ctx->fd >= 0) {
		if (fsync(ctx->fd)) {
			LFS_ERROR("fsync() failed: %d", errno);
			return LFS_ERR_IO;
		}
	}

	return LFS_ERR_OK;
}

#ifdef LFS_THREADSAFE
static int block_device_lock(const struct lfs_config *c)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	pthread_mutex_lock(&ctx->mutex);

	return LFS_ERR_OK;
}


static int block_device_unlock(const struct lfs_config *c)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	pthread_mutex_unlock(&ctx->mutex);

	return LFS_ERR_OK;
}
#endif


static void init_lfs_config(struct lfs_config *cfg, size_t blocksize, size_t blocks, void *ctx)
{
	memset(cfg, 0, sizeof(struct lfs_config));

	cfg->context = ctx;

	/* I/O callbacks */
	cfg->read = block_device_read;
	cfg->prog = block_device_prog;
	cfg->erase = block_device_erase;
	cfg->sync = block_device_sync;
#ifdef LFS_THREADSAFE
	cfg->lock = block_device_lock;
	cfg->unlock = block_device_unlock;
#endif
	/* LFS settings */
	cfg->read_size = 1;
	cfg->prog_size = blocksize;
	cfg->block_size = blocksize;
	cfg->block_count = blocks;

	cfg->block_cycles = -1;
	cfg->cache_size = blocksize;
	cfg->lookahead_size = 32;

}


struct lfs_context* lfs_init_mem(void *base, size_t size, size_t blocksize)
{
	if (!base || blocksize < 1)
		return NULL;

	if (size % blocksize != 0) {
		LFS_ERROR("image size not multiple of blocksize");
		return NULL;
	}

	struct lfs_context *ctx = calloc(1, sizeof(struct lfs_context));
	if (!ctx) {
		LFS_ERROR("out of memory");
		return NULL;
	}

	ctx->fd = -1;
	ctx->base = base;
	ctx->offset = 0;
#ifdef LFS_THREADSAFE
	pthread_mutex_init(&ctx->mutex, NULL);
#endif

	init_lfs_config(&ctx->cfg, blocksize, size / blocksize, ctx);

	return ctx;
}


struct lfs_context* lfs_init_file(int fd, size_t offset, size_t size, size_t blocksize)
{
	if (fd < 0 || blocksize < 1) {
		LFS_ERROR("invalid arguments");
		return NULL;
	}

	if (size % blocksize != 0) {
		LFS_ERROR("image size not multiple of blocksize");
		return NULL;
	}

	struct stat st;
	if (fstat(fd, &st) != 0) {
		LFS_ERROR("failed to stat file");
		return NULL;
	}
	if ((off_t)(offset + size) > st.st_size) {
		LFS_ERROR("file too small");
		return NULL;
	}

	struct lfs_context *ctx = calloc(1, sizeof(struct lfs_context));
	if (!ctx) {
		LFS_ERROR("out of memory");
		return NULL;
	}

	ctx->fd = fd;
	ctx->base = NULL;
	ctx->offset = offset;

	init_lfs_config(&ctx->cfg, blocksize, size / blocksize, ctx);

	return ctx;
}

int lfs_change_blocksize(struct lfs_context *ctx, size_t size, size_t blocksize)
{
	if (!ctx || blocksize < 1)
		return -1;

	if (size % blocksize != 0) {
		LFS_ERROR("image size not multiple of blocksize");
		return -2;
	}

	ctx->cfg.prog_size = blocksize;
	ctx->cfg.block_size = blocksize;
	ctx->cfg.cache_size = blocksize;

	return 0;
}

void lfs_destroy_context(struct lfs_context *ctx)
{
	if (!ctx)
		return;

#ifdef LFS_THREADSAFE
	pthread_mutex_destroy(&ctx->mutex);
#endif
	memset(ctx, 0, sizeof(struct lfs_context));
	free(ctx);
}

