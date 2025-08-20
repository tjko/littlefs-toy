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
#include <sys/types.h>
#include <dirent.h>
#if defined(HAVE_GETOPT_H) && defined(HAVE_GETOPT_LONG)
#include <getopt.h>
#else
#include "getopt/getopt.h"
#endif
#include <lfs.h>

#include "lfs_driver.h"
#include "littlefs-toy.h"

#define PATHSEPARATOR "/"


int command = LFS_NONE;
int verbose_mode = 0;
int overwrite_mode = 0;
int direct_mode = 0;
char *image_file = NULL;
char *directory = NULL;
lfs_size_t block_size = 4096;
uint32_t image_size = 0;
uint32_t image_offset = 0;

static const struct option long_options[] = {
	{ "create",             0, NULL,                'c' },
	{ "update",             0, NULL,                'r' },
	{ "delete",             0, NULL,                'd' },
	{ "list",               0, NULL,                't' },
	{ "file",               1, NULL,                'f' },
	{ "block-size",         1, NULL,                'b' },
	{ "size",               1, NULL,                's' },
	{ "offset",             1, NULL,                'o' },
	{ "directory",          1, NULL,                'C' },
	{ "help",		0, NULL,		'h' },
	{ "verbose",		0, NULL,		'v' },
	{ "version",		0, NULL,		'V' },
	{ "overwrite",          0, NULL,                'O' },
	{ "direct",             0, &direct_mode,         1 },
	{ NULL, 0, NULL, 0 }
};

static const char *copyright = "Copyright (C) 2025 Timo Kokkonen";

typedef struct param_t {
	const char *name;
	bool found;
	struct param_t *next;
} param_t;



const char* strip_path_prefix(const char *pathname)
{
	const char *p = pathname;
	const char *t;

	if (p) {
		while ((t = strstr(p, "/../"))) {
			p = t + 4;
		}
		while (p[0] == '.' || p[0] == '/') {
			if (p[0] == '.') {
				if (p[1] == '/')
					p += 2;
				else if (p[1] == '.' && p[2] == '/')
					p += 3;
				else
					break;
			} else {
				p++;
			}
		}
	}

	return p;
}

int parse_params(int argc, char **argv, int start, param_t **list, bool filecheck)
{
	int res = 0;
	int idx = start;
	param_t *tail = NULL;
	param_t *p;
	char fullname[LFS_NAME_MAX * 2];

	*list = NULL;

	while (idx < argc) {
		char *arg = argv[idx++];
		char prefix[3] = "./";

		if (filecheck) {
			if (!file_exists(arg)) {
				warn("%s: no such file or directory", arg);
				res++;
				continue;
			}
			prefix[0] = 0;
		}
		else {
			if ((arg[0] == '.' || arg[0] == '/') && arg[1] == 0) {
				arg = "./";
				prefix[0] = 0;
			}
			if (arg[0] == '/')
				prefix[1] = 0;
			else if (arg[0] == '.' && arg[1] == '/')
				prefix[0] = 0;
			else if (arg[0] == '.' && arg[1] == '.' && arg[2] == '/')
				arg++;
		}

		snprintf(fullname, sizeof(fullname), "%s%s", prefix, arg);

		if (!(p = calloc(1, sizeof(param_t))))
			fatal("out of memory");
		p->name = strdup(fullname);

		if (*list == NULL)
			*list = p;
		if (tail)
			tail->next = p;
		tail = p;
	}

	return res;
}


bool match_param(const char *name, param_t *list)
{
	if (!name || !list)
		return false;

	param_t *p = list;
	while (p) {
		if (!strcmp(p->name, "./") || !strcmp(name, p->name)) {
			p->found = true;
			return true;
		}
		p = p->next;
	}

	return false;
}


int littlefs_list(lfs_t *lfs, const char *path, bool recursive, param_t *params, bool match_all)
{
	lfs_dir_t dir;
	struct lfs_info info;
	char separator[2] = "/";
	char fullname[LFS_NAME_MAX * 2];
	size_t path_len;
	int res;

	if (!lfs || !path)
		return -1;

	/* Check if path ends with "/" ... */
	path_len = strnlen(path, LFS_NAME_MAX);
	if (path_len > 0) {
		if (path[path_len - 1] == '/')
			separator[0] = 0;
	}


	/* Open directory */
	if ((res = lfs_dir_open(lfs, &dir, path)) != LFS_ERR_OK)
		return -2;

	/* Read directory entries... */
	while ((res = lfs_dir_read(lfs, &dir, &info)) > 0) {
		bool skip = false;

		/* Skip special directories ("." and "..") */
		if (info.name[0] == '.') {
			if (info.name[1] == 0)
				continue;
			if (info.name[1] == '.' && info.name[2] == 0)
				continue;
		}

		snprintf(fullname, sizeof(fullname), "%s%s%s", path, separator, info.name);
		fullname[LFS_NAME_MAX] = 0;

		if (params && !match_all) {
			if (!match_param(fullname, params))
				skip = true;
		}

		if (!skip) {
			if (verbose_mode) {
				printf("%crw-rw-rw- root/root %9u 0000-00-00 00:00 %s%s%s\n",
					(info.type == LFS_TYPE_DIR ? 'd' : '-'),
					info.size,
					path, separator, info.name);
			}
			else {
				printf("%s%s%s\n", path, separator, info.name);
			}
		}

		if (info.type == LFS_TYPE_DIR && recursive) {
			littlefs_list(lfs, fullname, recursive, params, !skip);
		}
	}

	/* Close directory */
	lfs_dir_close(lfs, &dir);

	return 0;
}


int make_dir(lfs_t *lfs, const char *pathname)
{
	struct lfs_info info;
	char *path, *tok, *saveptr;
	int res = 0;


	if (!lfs || !pathname)
		return -1;

	if (lfs_stat(lfs, pathname, &info) == LFS_ERR_OK)
		return 0;

	if (!(path = strdup(pathname)))
		return -2;

	if ((tok = strtok_r(path, "/", &saveptr))) {
		while (tok) {
			/* printf("dir '%s'\n", path); */
			res = lfs_mkdir(lfs, path);
			if (res != LFS_ERR_EXIST && res != LFS_ERR_OK) {
				res = -4;
				break;
			}
			*(saveptr - 1) = '/';
			tok = strtok_r(NULL, "/", &saveptr);
		}
	} else {
		res = -3;
	}

	free(path);

	return res;
}



#define COPY_BUF_SIZE (1024 * 1024)

int copy_file_in(lfs_t *lfs, const char *pathname, bool overwrite)
{
	lfs_file_t file;
	struct lfs_info info;
	int res = 0;
	int fd;
	const char *newpath;
	char *dirname, *p;
	void *buf;
	ssize_t len;


	if (!lfs || !pathname)
		return -1;

	newpath = strip_path_prefix(pathname);

	if (!overwrite) {
		if (lfs_stat(lfs, newpath, &info) == LFS_ERR_OK) {
			warn("%s: file already exists on the filesystem", newpath);
			return -2;
		}
	}

	if (verbose_mode)
		printf("%s\n", newpath);

	/* Create directory for the file */
	if (!(dirname = strdup(newpath)))
		return -3;
	if ((p = strrchr(dirname, '/'))) {
		*p = 0;
		if ((res = make_dir(lfs, dirname)))
			res = -4;
	}
	free(dirname);

	/* Create the file */
	if ((fd = open_file(pathname, true)) >= 0) {
		res = lfs_file_open(lfs, &file, newpath, LFS_O_WRONLY | LFS_O_CREAT);
		if (res == LFS_ERR_OK) {
			if ((buf = calloc(1, COPY_BUF_SIZE))) {
				while ((len = read(fd, buf, COPY_BUF_SIZE)) > 0) {
					if (lfs_file_write(lfs, &file, buf, len) < len) {
						res = -8;
						break;
					}
				}
				free(buf);
			} else {
				res = -7;
			}
			lfs_file_close(lfs, &file);
		} else {
			res =-6;
		}
		close(fd);
	}
	else {
		res = -5;
	}

	return res;
}


int copy_dir_in(lfs_t *lfs, const char *dirname, bool overwrite)
{
	DIR *dir;
	struct dirent *e;
	struct stat st;
	char separator[2] = "/";
	char fullname[PATH_MAX + 1];
	size_t dirname_len;
	int res = 0;


	/* Check if path ends with "/" ... */
	if ((dirname_len = strnlen(dirname, NAME_MAX)) > 0) {
		if (dirname[dirname_len - 1] == '/')
			separator[0] = 0;
	}

	/* Read directory entries */
	if (!(dir = opendir(dirname))) {
		warn("%s: failed to open directory", dirname);
		return -1;
	}
	while ((e = readdir(dir))) {
		/* Skip special directories ("." and "..") */
		if (e->d_name[0] == '.') {
			if (e->d_name[1] == 0)
				continue;
			if (e->d_name[1] == '.' && e->d_name[2] == 0)
				continue;
		}
		snprintf(fullname, PATH_MAX, "%s%s%s", dirname, separator, e->d_name);
		fullname[PATH_MAX] = 0;

		if (lstat(fullname, &st)) {
			warn("cannot stat file: %s", fullname);
			res = -2;
			break;
		} else {
			if (S_ISREG(st.st_mode)) {
				if ((res = copy_file_in(lfs, fullname, overwrite))) {
					warn("%s: failed to copy file (%d)", fullname, res);
					res = -3;
					break;
				}
			}
			else if (S_ISDIR(st.st_mode)) {
				if ((res = copy_dir_in(lfs, fullname, overwrite))) {
					res = -4;
					break;
				}
			}
			else {
				warn("%s: skip special file", fullname);
			}
		}
	}
	closedir(dir);

	return res;
}

int littlefs_add(lfs_t *lfs, param_t *params, bool overwrite)
{
	struct stat st;
	int res = 0;

	if (params) {
		param_t *p = params;
		while (p) {
			if (lstat(p->name, &st)) {
				warn("cannot stat file: %s", p->name);
			} else {
				if (S_ISREG(st.st_mode)) {
					if ((res = copy_file_in(lfs, p->name, overwrite))) {
						warn("%s: failed to copy file (%d)", p->name, res);
						break;
					}
				}
				else if (S_ISDIR(st.st_mode)) {
					if ((res = copy_dir_in(lfs, p->name, overwrite))) {
						break;
					}
				}
				else {
					warn("%s: skip special file", p->name);
				}
			}
			p = p->next;
		}
	}
	else {
		warn("no files added to filesystem");
		res = -42;
	}

	return res;
}



int delete_dir(lfs_t *lfs, const char *pathname)
{
	struct lfs_info st;
	lfs_dir_t dir;
	char separator[2] = "/";
	char fullname[LFS_NAME_MAX * 2];
	size_t path_len;
	int res = 0;


	if (lfs_stat(lfs, pathname, &st) != LFS_ERR_OK)
		return -1;
	if (st.type != LFS_TYPE_DIR)
		return -2;

	/* Check if path ends with "/" ... */
	path_len = strnlen(pathname, LFS_NAME_MAX);
	if (path_len > 0) {
		if (pathname[path_len - 1] == '/')
			separator[0] = 0;
	}

	/* Read and delete entries in the directory */
	if (lfs_dir_open(lfs, &dir, pathname) != LFS_ERR_OK)
		return -3;
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
			if ((res = delete_dir(lfs, fullname))) {
				warn("%s: failed to remove subdirectory (%s)", fullname, res);
				res = 1;
				break;
			}
		} else {
			if ((res = lfs_remove(lfs, fullname)) != LFS_ERR_OK) {
				warn("%s: failed to remove file (%s):", fullname, res);
				res = 1;
				break;
			}
		}
		if (verbose_mode)
			printf("%s\n", fullname);
	}
	lfs_dir_close(lfs, &dir);

	if (res == 0) {
		res = lfs_remove(lfs, pathname);
	}

	return res;
}


int littlefs_del(lfs_t *lfs, param_t *params)
{
	struct lfs_info st;
	int res = 0;

	if (params) {
		param_t *p = params;
		while (p) {
			if (lfs_stat(lfs,p->name, &st) == LFS_ERR_OK) {
				p->found = true;
				if (st.type == LFS_TYPE_DIR) {
					if ((res = delete_dir(lfs, p->name))) {
						warn("%s: failed to remove directory (%d)", p->name, res);
						res = 1;
						break;
					}
				}
				else {
					if ((res = lfs_remove(lfs, p->name)) != LFS_ERR_OK) {
						warn("%s: failed to remove file (%d)", p->name, res);
						res = 1;
						break;
					}
				}
				if (verbose_mode)
					printf("%s\n", p->name);

			}
			p = p->next;
		}

		p = params;
		while (p) {
			if (!p->found) {
				warn("%s: not found in the filesystem", p->name);
				res = 2;
			}
			p = p->next;
		}
	}
	else {
		warn("no files to delete from filesystem");
		res = -42;
	}

	return res;
}


int littlefs_mount(struct lfs_context *ctx, lfs_t *lfs)
{
	lfs_size_t new_block_size = 0;
	int res = 0;
	char *msg;


	if (!ctx || !lfs)
		return -1;

	warn_clear_last_msg();
	warn_mode(false);
	if ((res = lfs_mount(lfs, &ctx->cfg)) == LFS_ERR_OK)
		return 0;
	warn_mode(true);

	/* I mount failed, check if blocksize was incorrect */
	if ((msg = strdup(warn_last_msg()))) {
		if (strstr(msg, "Invalid block size (")) {
			char *s, *e;
			if ((s = strrchr(msg, '('))) {
				s++;
				if ((e = strchr(s, ' ')))
					*e = 0;
				new_block_size = atoi(s);
			}
		}
	}

	if (new_block_size >= 128 && new_block_size <= 65536) {
		warn("warning: filesystem blocksize is %u (and not %u)",
			new_block_size, block_size);
		block_size = new_block_size;
		lfs_change_blocksize(ctx, image_size, block_size);
		res = lfs_mount(lfs, &ctx->cfg);
	}
	else {
		if (msg && strlen(msg) > 0)
			warn(msg);
	}
	if (msg)
		free(msg);

	return res;
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
		"  -c, --create               Create (format) LFS image and add files\n"
		"  -r, --append               Append (add) files to existing LFS image\n"
		"  -d, --delete               Remove files from existing LFS image\n"
		"  -t, --list                 List contents of existing LFS image\n\n"
		" Options:\n"
		" -f <imagefile>, --file=<imagefile>\n"
                "                             Specify LFS image file location\n"
		" -b <blocksize>, --block-size=<blocksize>\n"
		"                             LFS filesystem blocksize (default: %d)\n"
		" -s <imagesize>, --size=<imagesize>\n"
		"                             LFS filesystem size (required with -c)\n"
		" -o <imageoffset>, --offset=<imageoffset>\n"
                "                             LFS filesystem start offset (default: 0)\n"
		" -h, --help                  Display usage information and exit\n"
		" -v, --verbose               Enable verbose mode\n"
		" -V, --version               Display program version\n"
		" -O, --overwrite             Overwrite image file (if it exists already)\n"
		" --direct                    Write to image file directly (use less memory)\n"
		"\n\n", 4096);
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
		if ((c = getopt_long(argc, argv, "crdtf:b:s:o:C:hvVO",
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
			if (parse_arg_val(optarg, &val, 128, ((int64_t)1 << 31))) {
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

		case 'C':
			if (directory)
				free(directory);
			directory = strdup(optarg);
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

		case 'O':
			overwrite_mode = 1;
			break;

		case '?':
			fprintf(stderr, "Try '%s --help' for more information.\n", PROGRAMNAME);
			exit(1);
		}
	}


	if (command == LFS_NONE) {
		warn("no command specified");
		fprintf(stderr, "Try '%s --help' for more information.\n", PROGRAMNAME);
		exit(1);
	}

	if (!image_file)
		fatal("no image file (-f <filename>) specified");

	if (command == LFS_CREATE) {
		if (image_size < 1)
			fatal("image size (-s <imagesize>) must be set when creating a new image");
	}

	return optind;
}


int main(int argc, char **argv)
{
	struct lfs_context *ctx;
	param_t *params = NULL;
	void *image_buf = NULL;
	lfs_t lfs;
	int fd = -1;
	int ret = 0;
	int res;


	parse_arguments(argc, argv);

	/* Open image file */
	if (!file_exists(image_file)) {
		if (command != LFS_CREATE)
			fatal("image file not found: %s", image_file);
		if ((fd = create_file(image_file, image_size + image_offset)) < 0)
			fatal("cannot create image file: %s", image_file);
	}
	else {
		if (!overwrite_mode && command == LFS_CREATE)
			fatal("image file already exists: %s", image_file);

		if ((fd = open_file(image_file, (command == LFS_LIST ? true : false))) < 0)
			fatal("cannot open image file: %s", image_file);

		if (command == LFS_CREATE) {
			off_t sz = file_size(fd);
			if (sz < 0)
				fatal("cannot determine file size: %s", image_file);
			if (direct_mode && sz < image_offset + image_size) {
				if ((res = file_set_zero(fd, image_offset, image_size)))
					fatal("failed to zero-out lfs image");
			}
		}
	}

	/* Change directory if -C, --directory option specified. */
	if (directory) {
		if (chdir(directory))
			fatal("cannot change directory to: %s", directory);
	}

	/* Initialize LittleFS library */
	if (direct_mode) {
		ctx = lfs_init_file(fd, image_offset, image_size, block_size);
	} else {
		ssize_t bufsize = image_size;

		if (bufsize == 0) {
			if ((bufsize = file_size(fd)) < 0)
				fatal("%s: cannot get file image file size", image_file);
			bufsize -= image_offset;
			if (bufsize < 0)
				fatal("invalid offset: %ld", image_offset);
		}
		if (!(image_buf = calloc(1, bufsize)))
			fatal("out of memory");
		if (command != LFS_CREATE) {
			if ((res = read_file(fd, image_offset, image_buf, bufsize)))
				fatal("%s: failed to read image from file (%d)", image_file, errno);
		}
		ctx = lfs_init_mem(image_buf, image_size, block_size);
	}
	if (!ctx)
		fatal("failed to initialize LittleFS");

	if (command == LFS_CREATE) {
		/* Make new filesystem */
		if ((res = lfs_format(&lfs, &ctx->cfg)) != LFS_ERR_OK)
			fatal("%s: failed to create a new LittleFS filesystem: %d", image_file, res);
	}

	/* Mount LittleFS */
#if 0
	block_size = 512;
	lfs_change_blocksize(ctx, image_size, block_size);
	if ((res = lfs_mount(&lfs, &ctx->cfg)) != LFS_ERR_OK)
		fatal("%s: failed to mount LittleFS (%d)", image_file, res);
#endif
	if ((res = littlefs_mount(ctx, &lfs)))
		fatal("%s: failed to mount LittleFS (%d)", image_file, res);

	if (image_size == 0) {
		image_size = block_size * lfs.block_count;
	} else {
		uint32_t new_size = block_size * lfs.block_count;
		if (image_size != new_size)
			warn("specified image size does not match filesystem: %u vs %u",
				image_size, new_size);
	}

	if (verbose_mode > 1) {
		lfs_size_t used_blocks = lfs_fs_size(&lfs);

		printf("Filesystem size: %10u bytes (%u blocks)\n", block_size * lfs.block_count,
			lfs.block_count);
		printf("           used: %10u bytes (%u blocks)\n", block_size * used_blocks,
			used_blocks);
		printf("           free: %10u bytes (%u blocks)\n\n",
			block_size * (lfs.block_count - used_blocks), lfs.block_count - used_blocks);
		printf("      blocksize: %10u bytes\n\n", block_size);
	}



	/* Parse command parameters */
	bool filecheck = (command == LFS_CREATE || command == LFS_UPDATE) ? true : false;
	if ((res = parse_params(argc, argv, optind, &params, filecheck))) {
		warn("failed to parse all parameters: %d", res);
		ret = 2;
	}


	/* Process command */
	switch (command) {

	case LFS_LIST:
		if (littlefs_list(&lfs, "./", true, params, false) > 0)
			ret = 1;
		param_t *p = params;
		while (p) {
			if (!p->found) {
				warn("%s: not found in the filesystem", p->name);
				ret = 2;
			}
			p = p->next;
		}
		break;

	case LFS_CREATE:
	case LFS_UPDATE:
		if (littlefs_add(&lfs, params, true))
			ret = 1;
		break;

	case LFS_DELETE:
		if (littlefs_del(&lfs, params))
			ret = 1;
		break;

	default:
		fatal("internal error");

	}


	/* Unmount LittleFS */
	if ((res = lfs_unmount(&lfs)) != LFS_ERR_OK)
		fatal("%s: failed to unmount LittleFS (%d)", image_file, res);

	if (!direct_mode && command != LFS_LIST) {
		/* Write image from memory to the image file */
		if ((res = write_file(fd, image_offset, image_buf, image_size)))
			fatal("%s: failed to write image to file (%d)", image_file, errno);
	}

	if (fd >= 0)
		close(fd);
	lfs_destroy_context(ctx);

	return ret;
}
