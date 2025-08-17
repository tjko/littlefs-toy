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

#include <stdio.h>
#include <string.h>
#include <getopt.h>
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


void print_version()
{
#ifdef  __DATE__
	printf("lfs v%s%s  %s (%s)\n", LITTLEFS_TOY_VERSION,
		BUILD_TAG, HOST_TYPE, __DATE__);
#else
	printf("lfs v%s%s  %s\n", LITTLEFS_TOY_VERSION,
		BUILD_TAG, HOST_TYPE);
#endif
	printf("%s\n\n", copyright);
	printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
		"and you are welcome to redistribute it under certain conditions.\n"
		"See the GNU General Public License for more details.\n\n");
}


void print_usage()
{
	fprintf(stderr, "lfs v" LITTLEFS_TOY_VERSION
		BUILD_TAG "  %s\n\n", copyright);

	fprintf(stderr, "Usage: lfs [OPTIONS] <operation> <parameter> ...\n\n"
		" -h, --help    display usage information and exit\n"
		" -v, --verbose enable verbose mode\n"
		" -V, --version display program version\n"
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
