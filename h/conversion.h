/*
 * Copyright 2012 Peter Bašista
 *
 * This file is part of rsgen
 *
 * rsgen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* this feature test macro enables the st_blksize member of the struct stat */
#define _XOPEN_SOURCE 500
/* a feature test macro, which enables the support for large files (> 2 GiB) */
#define _FILE_OFFSET_BITS 64

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iconv.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

/* simple typedefs */

typedef std::map<wchar_t, size_t> occurrences_map;

int text_file_read_buffer (int fd,
		size_t buffer_size,
		char *buffer,
		size_t *bytes_read);
int convert_buffer (iconv_t *cd,
		char *buffer,
		size_t buffer_size,
		wchar_t *wbuffer,
		size_t wbuffer_size,
		size_t *characters_read);
int add_character_occurrences(occurrences_map &occurrences,
		wchar_t *wbuffer,
		size_t wbuffer_size);
