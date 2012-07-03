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

#include "conversion.h"

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

/* auxiliary functions */

/**
 * A function which computes the greatest common divisor
 * of the two numbers provided.
 *
 * @param
 * a	the first number
 * @param
 * a	the second number
 *
 * @return	This function always returns the greatest common divisor
 * 		of the two provided numbers 'a' and 'b'.
 */
unsigned long long compute_gcd (unsigned long long a,
		unsigned long long b) {
	unsigned long long c = 0;
	for (;;) {
		c = a % b;
		if (c == 0) {
			return (b);
		}
		a = b;
		b = c;
	}
}

/**
 * A function which reads the next 'buffer_size'
 * characters from the file opened by a file descriptor 'fd'
 * into the 'buffer'.
 *
 * @param
 * fd	the file descriptor from which the buffer will be read
 * @param
 * buffer_size	the desired number of bytes to be read
 * @param
 * buffer	the buffer into which the next parts of the file will be read
 * @param
 * bytes_read	when this function returns, this variable will be set
 * 		to the number of bytes, which has actually been read
 * 		from the specified input file
 *
 * @return	If the desired number of bytes has been successfully read,
 * 		this function returns zero.
 * 		If the desired number of bytes could not have been
 * 		successfully read, because the end of the file
 * 		has been encountered, this function returns (-1).
 * 		Otherwise, in case of any error, this function returns (1).
 */
int text_file_read_buffer (int fd,
		size_t buffer_size,
		char *buffer,
		size_t *bytes_read) {
	ssize_t read_retval = read(fd, buffer, buffer_size);
	/* we check whether the read has encountered an error */
	if (read_retval == (-1)) {
		perror("text_file_read_buffer: read");
		/* resetting the errno */
		errno = 0;
		return (1); /* failure */
	/* if we have reached the end of the input file */
	} else if (read_retval == 0) {
		(*bytes_read) = 0;
		return (-1); /* partial success */
	}
	(*bytes_read) = (size_t)(read_retval);
	return (0); /* success */
}

/**
 * A function which reads the 'buffer_size' bytes from the input buffer
 * of standard characters and converts them using the iconv function
 * initialized by the provided conversion descriptor to the wide characters
 * which are then stored in the provided buffer of wide characters.
 *
 * @param
 * cd	the iconv conversion descriptor used for the character conversion
 * @param
 * buffer	the input buffer of standard characters
 * @param
 * buffer_size	the number of bytes in the input buffer
 * @param
 * wbuffer	the output buffer of wide characters
 * @param
 * wbuffer_size	the number of characters in the output buffer
 * @param
 * characters_read	when this function returns, this variable will be set
 * 			to the number of characters, which has actually been read
 * 			from the specified input buffer
 *
 * @return	If the entire input buffer has been successfully converted,
 * 		this function returns zero.
 * 		Otherwise, a positive error number is returned.
 */
int convert_buffer (iconv_t *cd,
		char *buffer,
		size_t buffer_size,
		wchar_t *wbuffer,
		size_t wbuffer_size,
		size_t *characters_read) {
	char *inbuf = buffer;
	char *outbuf = (char *)(wbuffer);
	size_t inbytesleft = buffer_size;
	size_t outbytesleft = wbuffer_size * sizeof (wchar_t);
	size_t outbytesleft_at_start = outbytesleft;
	size_t retval = 0;
	/*
	 * we try to use iconv to convert the characters
	 * in the input buffer to the characters in the output buffer
	 */
	retval = iconv((*cd), &inbuf, &inbytesleft,
			&outbuf, &outbytesleft);
	/* if the iconv has encountered an error */
	if (retval == (size_t)(-1)) {
		perror("text_file_convert_buffer: iconv");
		/* resetting the errno */
		errno = 0;
		return (1);
	} else if (retval > 0) {
		std::cerr << "text_file_convert_buffer: iconv "
			"converted " << retval << " characters\n"
			"in a nonreversible way!\n";
		return (2);
	} else if (outbytesleft == 0) {
		/*
		 * all the characters expected to be read
		 * from the input buffer have already been read
		 */
	} else if (inbytesleft > 0) {
		std::cerr << "text_file_convert_buffer: iconv could not "
			"convert " << inbytesleft << " input bytes!\n";
		return (3);
	}
	/*
	 * now we compute the number of characters,
	 * which have just been converted from the input buffer
	 */
	(*characters_read) = (outbytesleft_at_start -
			outbytesleft) / sizeof (wchar_t);
	return (0);
}

/**
 * A function which scans the provided buffer of wide characters
 * and adds the numbers of their respective occurrences
 * to the provided occurrence map
 *
 * @param
 * occurrences	a std::map containing the current numbers
 * 		of character occurrences
 * @param
 * wbuffer	the buffer of wide characters
 * @param
 * wbuffer_size	the size of the input buffer of wide characters
 *
 * @return	This function always returns zero.
 */
int add_character_occurrences(occurrences_map &occurrences,
		wchar_t *wbuffer,
		size_t wbuffer_size) {
	size_t i = 0;
	for (; i < wbuffer_size; ++i) {
		/*
		 * FIXME: We are strongly relying on
		 * the size_t being initialized to zero!
		 * How portable is this?
		 */
		++occurrences[wbuffer[i]];
	}
	return (0);
}
