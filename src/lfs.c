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

int verbose_mode = 0;

#define VERSION "0.0.1"
#define HOST_TYPE "Unknown"

const struct option long_options[] = {
	{ "help",		0, NULL,		'h' },
	{ "verbose",		0, NULL,		'v' },
	{ "version",		0, NULL,		'V' },
	{ NULL, 0, NULL, 0 }
};

const char *copyright = "Copyright (C) 2025 Timo Kokkonen.";


void print_version()
{
#ifdef  __DATE__
	printf("lfs v%s  %s (%s)\n", VERSION, HOST_TYPE, __DATE__);
#else
	printf("lfs v%s  %s\n", VERSION, HOST_TYPE);
#endif
	printf("%s\n\n", copyright);
	printf("This program comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
		"and you are welcome to redistribute it under certain conditions.\n"
		"See the GNU General Public License for more details.\n\n");
}


void print_usage()
{
	fprintf(stderr, "lfs v" VERSION "  %s\n\n", copyright);

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
		if ((c = getopt_long(argc, argv, "hvV",
						long_options, &opt_index)) == -1)
			break;

		switch (c) {

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

	return opt_index;
}


int main(int argc, char **argv)
{

	int res = parse_arguments(argc, argv);

	printf("res=%d %d\n", res, optind);

	return 0;
}
