/* lfs.c
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
#include <sys/errno.h>
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#else
#include "getopt/getopt.h"
#endif
#include <lfs.h>

#include "littlefs-toy.h"



int command = LFS_NONE;
int verbose_mode = 0;
char *image_file = NULL;
lfs_size_t block_size = 4096;
uint32_t image_size = 0;
uint32_t image_offset = 0;

const struct option long_options[] = {
	{ "create",             0, &command,            LFS_CREATE },
	{ "update",             0, &command,            LFS_UPDATE },
	{ "delete",             0, &command,            LFS_DELETE },
	{ "list",               0, &command,            LFS_LIST },
	{ "file",               1, NULL,                'f' },
	{ "block-size",         1, NULL,                'b' },
	{ "size",               1, NULL,                's' },
	{ "offset",             1, NULL,                'o' },
	{ "help",		0, NULL,		'h' },
	{ "verbose",		0, NULL,		'v' },
	{ "version",		0, NULL,		'V' },
	{ NULL, 0, NULL, 0 }
};

const char *copyright = "Copyright (C) 2025 Timo Kokkonen.";

struct lfs_context {
	struct lfs_config cfg;
	int fd;
	void *base;
	size_t offset;
};


static int block_read(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, void *buffer, lfs_size_t size)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;
//	printf("block_read(%p,%u,%u,%p,%u)\n", c, block, off, buffer, size);

	if (c->block_count > 0 && block >= c->block_count) {
		warn("attempt to read past end of filesystem");
		return LFS_ERR_IO;
	}
	if (off + size > c->block_size) {
		warn("attempt to read past end of block");
		return LFS_ERR_IO;
	}

	if (ctx->fd >= 0) {
		off_t f_offset = ctx->offset + (block * c->block_size) + off;
		if (lseek(ctx->fd, f_offset, SEEK_SET) < 0) {
			warn("seek failed: %ld (errno=%d)", f_offset, errno);
			return LFS_ERR_IO;
		}
		if (read(ctx->fd, buffer, size) < size) {
			warn("failed to read file");
			return LFS_ERR_IO;
		}
	} else {
		memcpy(buffer, ctx->base + (block * c->block_size) + off, size);
	}
	return LFS_ERR_OK;
}

static int block_prog(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, const void *buffer, lfs_size_t size)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	if (block >= c->block_count) {
		warn("attempt to write past end of filesystem");
		return LFS_ERR_IO;
	}
	if (off % c->prog_size != 0) {
		warn("flash addres must be aligned to flash page");
		return LFS_ERR_IO;
	}
	if (size % c->prog_size != 0) {
		warn("bytes to write must be multiple of flash page size");
		return LFS_ERR_IO;
	}
	if (off + size > c->block_size) {
		warn("write must be within a block");
		return LFS_ERR_IO;
	}

	if (ctx->fd >= 0) {
		off_t f_offset = ctx->offset + (block * c->block_size) + off;
		if (lseek(ctx->fd, f_offset, SEEK_SET) < 0) {
			warn("seek failed: %ld", f_offset);
			return LFS_ERR_IO;
		}
		if (write(ctx->fd, buffer, size) < size) {
			warn("failed to read file");
			return LFS_ERR_IO;
		}
	} else {
		memcpy(ctx->base + (block * c->block_size) + off, buffer, size);
	}

	return LFS_ERR_OK;
}

static int block_erase(const struct lfs_config *c, lfs_block_t block)
{
	return LFS_ERR_OK;
}

static int block_sync(const struct lfs_config *c)
{
	struct lfs_context *ctx = (struct lfs_context*)c->context;

	if (ctx->fd >= 0) {
		if (fsync(ctx->fd)) {
			warn("fsync() failed: %d", errno);
			return LFS_ERR_IO;
		}
	}

	return LFS_ERR_OK;
}

static void init_lfs_config(struct lfs_config *cfg, size_t blocksize, size_t blocks, void *ctx)
{
	memset(cfg, 0, sizeof(struct lfs_config));

	cfg->context = ctx;

	/* I/O callbacks */
	cfg->read = block_read;
	cfg->prog = block_prog;
	cfg->erase = block_erase;
	cfg->sync = block_sync;

	/* LFS settings */
	cfg->read_size = 1;
	cfg->prog_size = blocksize;
	cfg->block_size = blocksize;
	cfg->block_count = blocks;

	cfg->block_cycles = 300;
	cfg->cache_size = blocksize;
	cfg->lookahead_size = 32;

}

struct lfs_context* lfs_init_mem(void *base, size_t size, size_t blocksize)
{
	if (!base || size < 1 || blocksize < 1)
		return NULL;

	if (size % blocksize != 0) {
		warn("image size not multiple of blocksize");
		return NULL;
	}
	if (size < blocksize) {
		warn("image smaller than blocksize");
		return NULL;
	}

	struct lfs_context *ctx = calloc(1, sizeof(struct lfs_context));
	if (!ctx) {
		warn("out of memory");
		return NULL;
	}

	ctx->fd = -1;
	ctx->base = base;
	ctx->offset = 0;

	init_lfs_config(&ctx->cfg, blocksize, size / blocksize, ctx);

	return ctx;
}


struct lfs_context* lfs_init_file(int fd, size_t offset, size_t size, size_t blocksize)
{
	if (fd < 0 || blocksize < 1) {
		warn("invalid arguments");
		return NULL;
	}

	if (offset % blocksize != 0) {
		warn("offset not multiple of blocksize");
		return NULL;
	}
	if (size % blocksize != 0) {
		warn("image size not multiple of blocksize");
		return NULL;
	}

	struct stat st;
	if (fstat(fd, &st) != 0) {
		warn("failed to stat file");
		return NULL;
	}
	if (offset + size > st.st_size) {
		warn("file too small");
		return NULL;
	}

	struct lfs_context *ctx = calloc(1, sizeof(struct lfs_context));
	if (!ctx) {
		warn("out of memory");
		return NULL;
	}

	ctx->fd = fd;
	ctx->base = NULL;
	ctx->offset = offset;

	init_lfs_config(&ctx->cfg, blocksize, size / blocksize, ctx);

	return ctx;
}



static int littlefs_list_dir(lfs_t *lfs, const char *path, bool recursive)
{
	lfs_dir_t dir;
	struct lfs_info info;
	char separator[2] = "/";
	char *dirname;
	size_t path_len;
	int res;

	if (!path)
		return LFS_ERR_INVAL;

	/* Check if path ends with "/" ... */
	path_len = strnlen(path, LFS_NAME_MAX);
	if (path_len > 0) {
		if (path[path_len - 1] == '/')
			separator[0] = 0;
	}

	printf("Directory: %s\n\n", path);

	/* Open directory */
	if ((res = lfs_dir_open(lfs, &dir, path)) != LFS_ERR_OK)
		return res;

	/* First scan for directories... */
	while ((res = lfs_dir_read(lfs, &dir, &info)) > 0) {
		if (info.type == LFS_TYPE_DIR)
			printf("%-50s %10s\n", info.name, "<DIR>");
	}

	/* Next scan for files... */
	lfs_dir_rewind(lfs, &dir);
	while ((res = lfs_dir_read(lfs, &dir, &info)) > 0) {
		if (info.type == LFS_TYPE_REG)
			printf("%-50s %10u\n", info.name, info.size);
	}
	printf("\n");

	if (recursive) {
		/* Scan subdirectories recursively... */
		lfs_dir_rewind(lfs, &dir);
		while ((res = lfs_dir_read(lfs, &dir, &info)) > 0) {
			if (info.type != LFS_TYPE_DIR)
				continue;
			/* Skip special directories ("." and "..") */
			if (info.name[0] == '.') {
				if (info.name[1] == 0)
					continue;
				if (info.name[1] == '.' && info.name[2] == 0)
					continue;
			}
			if ((dirname = malloc(LFS_NAME_MAX + 1))) {
				snprintf(dirname, LFS_NAME_MAX + 1, "%s%s%s",
					path, separator, info.name);
				littlefs_list_dir(lfs, dirname, recursive);
				free(dirname);
			}
		}
	}

	lfs_dir_close(lfs, &dir);

	return LFS_ERR_OK;
}




void print_version()
{
#ifdef  __DATE__
	printf("%s v%s%s  %s (%s)\n", PROGRAMNAME, LITTLEFS_TOY_VERSION,
		BUILD_TAG, HOST_TYPE, __DATE__);
#else
	printf("%d v%s%s  %s\n", PROGRAMNAME, LITTLEFS_TOY_VERSION,
		BUILD_TAG, HOST_TYPE);
#endif
	printf("%s\n\n", copyright);
	printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
		"and you are welcome to redistribute it under certain conditions.\n"
		"See the GNU General Public License for more details.\n\n");
}


void print_usage()
{
#if 0
	fprintf(stderr, "%s v%s%s %s\n\n", PROGRAMNAME,
		LITTLEFS_TOY_VERSION, BUILD_TAG, copyright);
#endif

	fprintf(stderr, "Usage: lfs {command} [options] [(file) | (pattern) ...]\n\n"
		" Commands:\n"
		"  -c, --create    Create (format) lfs image and add files\n"
		"  -r, --append    Append (add) files to existing lfs image\n"
		"  -d, --delete    Remove files from lfs image\n"
		"  -t, --list      List contents of lfs image\n\n"
		" Options:\n"
		" -f <imagefile>, --file=<imagefile>       lfs image file\n"
		" -b <blocksize>, --block-size=<blocksize> lfs block size\n"
		" -h, --help                               display usage information and exit\n"
		" -v, --verbose                            enable verbose mode\n"
		" -V, --version                            display program version\n"
		"\n\n");
}


int parse_arg_val(const char *str, int64_t *val, int64_t min, int64_t max)
{
	if (!str || !val)
		return -1;

	char *endptr;
	int64_t v = strtoll(str, &endptr, 10);

	if (str == endptr)
		return 1;
	if (*endptr != 0)
		return 2;
	if (v < min || v > max)
		return 3;

	*val = v;
	return 0;
}


int parse_arguments(int argc, char **argv)
{
	int c, opt_index;
	int64_t val;

	while (1) {
		opt_index = 0;
		if ((c = getopt_long(argc, argv, "crdtf:b:s:o:hvV",
						long_options, &opt_index)) == -1)
			break;

		switch (c) {

		case 'c':
			command = LFS_CREATE;
			break;

		case 'r':
			command = LFS_UPDATE;
			break;

		case 'd':
			command = LFS_DELETE;
			break;

		case 't':
			command = LFS_LIST;
			break;

		case 'f':
			image_file = strdup(optarg);
			break;

		case 'b':
			if (parse_arg_val(optarg, &val, 104, ((int64_t)1 << 31))) {
				fatal("invalid block-size specified: %s", optarg);
			}
			block_size = val;
			break;

		case 's':
			if (parse_arg_val(optarg, &val, 0, ((int64_t)1 << 32))) {
				fatal("invalid filesystem size specified: %s", optarg);
			}
			image_size = val;
			break;

		case 'o':
			if (parse_arg_val(optarg, &val, 0, ((int64_t)1 << 32))) {
				fatal("invalid filesystem offset specified: %s", optarg);
				exit(1);
			}
			image_offset = val;
			break;

		case 'h':
			print_usage();
			exit(0);

		case 'v':
			verbose_mode++;
			break;

		case 'V':
			print_version();
			exit(0);

		case '?':
			exit(1);
		}
	}


	if (command == LFS_NONE) {
		fprintf(stderr, "No command specified\n\n");
		print_usage();
		exit(1);
	}

	if (!image_file) {
		fprintf(stderr, "No image file specified\n\n");
		print_usage();
		exit(1);
	}

	return optind;
}


int main(int argc, char **argv)
{
	struct lfs_context *ctx;
	lfs_t lfs;
	int fd = -1;

	int res = parse_arguments(argc, argv);

#if 0
	printf("image file: %s\n", image_file);
	printf("block-size: %u\n", block_size);
	printf("size: %u\n", image_size);
	printf("offset: %u\n", image_offset);
#endif

	/* Open image file */
	if (!file_exists(image_file)) {
		if (command != LFS_CREATE)
			fatal("image file not found: %s", image_file);
		if (image_size < 1)
			fatal("image size must be specified to create image");
		if ((fd = create_file(image_file, image_size + image_offset)) < 0)
			fatal("cannot create image file: %s", image_file);
	}
	else {
		if ((fd = open_file(image_file, (command == LFS_LIST ? true : false))) < 0)
			fatal("cannot open image file: %s", image_file);
	}


	/* Mount LittleFS */
	if (!(ctx = lfs_init_file(fd, image_offset, 0, block_size)))
		fatal("Failed to initialize LittleFS");
	if ((res = lfs_mount(&lfs, &ctx->cfg)) != LFS_ERR_OK)
		fatal("Failed to mount LittleFS: %d", res);

	lfs_size_t used_blocks = lfs_fs_size(&lfs);

	if (verbose_mode) {
		printf("Filesystem size: %10u bytes (%u blocks)\n", block_size * lfs.block_count,
			lfs.block_count);
		printf("           used: %10u bytes (%u blocks)\n", block_size * used_blocks,
			used_blocks);
		printf("           free: %10u bytes (%u blocks)\n",
			block_size * (lfs.block_count - used_blocks), lfs.block_count - used_blocks);
		printf("     block size: %10u bytes\n\n", block_size);
	}

	if (command == LFS_LIST) {
		littlefs_list_dir(&lfs, "/", true);
	}


	/* Unmount LittleFS */
	if ((res = lfs_unmount(&lfs)) != LFS_ERR_OK)
		fatal("Failed to unmount LittleFD: %d", res);

	if (fd >= 0)
		close(fd);

	return 0;
}
