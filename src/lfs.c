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

const struct option long_options[] = {
	{ "create",             0, &command,            LFS_CREATE },
	{ "update",             0, &command,            LFS_UPDATE },
	{ "delete",             0, &command,            LFS_DELETE },
	{ "list",               0, &command,            LFS_LIST },
	{ "file",               1, NULL,                'f' },
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
	return LFS_ERR_OK;
}

static int block_prog(const struct lfs_config *c, lfs_block_t block,
		lfs_off_t off, const void *buffer, lfs_size_t size)
{
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
	if (fd < 0 || size < 1 || blocksize < 1)
		return NULL;

	if (offset % blocksize != 0) {
		warn("offset not multiple of blocksize");
		return NULL;
	}
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

	ctx->fd = fd;
	ctx->base = NULL;
	ctx->offset = offset;

	init_lfs_config(&ctx->cfg, blocksize, size / blocksize, ctx);

	return ctx;
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
	fprintf(stderr, "%s v%s%s %s\n\n", PROGRAMNAME,
		LITTLEFS_TOY_VERSION, BUILD_TAG, copyright);

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


int parse_arguments(int argc, char **argv)
{
	int c, opt_index;


	while (1) {
		opt_index = 0;
		if ((c = getopt_long(argc, argv, "crdtf:hvV",
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
		fprintf(stderr, "no command specified\n");
		print_usage();
		exit(1);
	}
	return opt_index;
}


int main(int argc, char **argv)
{

	int res = parse_arguments(argc, argv);

	printf("res=%d %d\n", res, optind);

	return 0;
}
