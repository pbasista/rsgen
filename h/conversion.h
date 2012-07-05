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
		char *buffer,
		size_t buffer_size,
		size_t *bytes_read);
int convert_from_wbuffer (iconv_t *cd,
		wchar_t *input_buffer,
		char *output_buffer,
		size_t input_buffer_size,
		size_t output_buffer_size,
		size_t *written_bytes);
int convert_to_wbuffer (iconv_t *cd,
		char *input_buffer,
		wchar_t *output_buffer,
		size_t input_buffer_size,
		size_t output_buffer_size,
		size_t *written_characters);
int add_character_occurrences(occurrences_map &occurrences,
		wchar_t *wbuffer,
		size_t wbuffer_size);
