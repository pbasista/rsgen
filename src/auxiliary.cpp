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

#include "auxiliary.h"

#include <cerrno>
#include <climits>
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

/* member functions */

rsgen *rsgen::instance (const int prng_type = 1) {
	static rsgen my_static_instance(prng_type);
	if (my_instance == NULL) {
		my_instance = &my_static_instance;
	}
	return (my_instance);
}

rsgen *rsgen::get_instance () {
	return (my_instance);
}

unsigned int rsgen::next () {
	unsigned int pseudorandom_number = 0;
	ssize_t read_retval = 0;
	switch (prng_type) {
		case 1 : /* Mersenne twister */
			pseudorandom_number = (unsigned int)
				(mprng->BRandom());
			break;
		case 2 : /* the random() function */
			/*
			 * since random() generates only non-negative ints
			 * as random numbers, we have to use it twice
			 * in order to cover the entire range of unsigned int
			 */
			if (random() % 2 == 0) {
				pseudorandom_number = (unsigned int)
					(random());
			} else {
				pseudorandom_number = (unsigned int)
					(random()) + ((unsigned int)(1) << 31);
			}
			break;
		case 3 : /* the /dev/urandom system file */
			read_retval = read(ufd,
					&pseudorandom_number,
					sizeof (unsigned int));
			/*
			 * we check whether the read
			 * has encountered an error
			 */
			if (read_retval == (-1)) {
				perror("rsgen::next(): read");
				/* resetting the errno */
				errno = 0;
				throw my_exception();
			/* if we have reached the end of the input file */
			} else if (read_retval == 0) {
				throw my_exception();
			}
			break;
		default :
			std::cerr << "Unknown value "
				"of the supplied prng_type (" <<
				prng_type << ") encountered!\n";
			throw my_exception();
	}
	return (pseudorandom_number);
}

rsgen::rsgen (const int prng_type_arg = 1) : prng_type(prng_type_arg) {
	switch (prng_type) {
		case 1 : /* Mersenne twister */
			mprng = new CRandomMersenne((int)(time(NULL)));
			break;
		case 2 : /* the random() function */
			srandom((unsigned int)(time(NULL)));
			break;
		case 3 : /* the /dev/urandom system file */
			/*
			 * we try to open the /dev/urandom
			 * system file for reading
			 */
			ufd = open("/dev/urandom", O_RDONLY);
			if (ufd == (-1)) {
				perror("/dev/urandom: open");
				throw /* FIXME WHAT? */;
			}
			break;
		default :
			std::cerr << "Unknown value "
				"of the supplied prng_type (" <<
				prng_type << ") encountered!\n";
			throw my_exception();
	}
}

rsgen::rsgen (const rsgen &rhs) {
}

rsgen &rsgen::operator= (const rsgen &rhs) {
	return (*this);
}

rsgen::~rsgen () {
	/*
	 * we strongly want to avoid the propagation
	 * of any exceptions out of the destructor
	 */
	try {
		delete mprng;
		if (close(ufd) == -1) {
			perror("/dev/urandom: close");
		}
	} catch (...) {
	}
}

/* static private member variables */

rsgen *rsgen::my_instance = NULL;

/* regular functions */

/**
 * A function which reads the next 'buffer_size'
 * characters from the file opened by a file descriptor 'fd'
 * into the 'buffer'.
 *
 * @param
 * fd	the file descriptor from which the buffer will be read
 * @param
 * buffer	the buffer into which the next parts of the file will be read
 * @param
 * buffer_size	the desired number of bytes to be read
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
		char *buffer,
		size_t buffer_size,
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
	} else {
		(*bytes_read) = (size_t)(read_retval);
		if ((*bytes_read) < buffer_size) {
			return (-1); /* partial success */
		} else {
			return (0); /* success */
		}
	}
}

/**
 * A function, which prepares the output buffer
 * and fills it with the appropriately converted characters
 * from the supplied buffer of wide characters.
 *
 * @param
 * cd		the iconv conversion descriptor used by the iconv
 * 		for character conversion
 * @param
 * input_buffer	the input buffer of wide characters, which will be converted
 * 		and written out to the output buffer of bytes
 * @param
 * output_buffer	the output buffer, which will be filled
 * 			with bytes corresponding to the converted characters
 * 			present in the input_buffer
 * @param
 * input_buffer_size	the number of wchar_t characters in the input_buffer
 * @param
 * output_buffer_size	the number of bytes in the output_buffer
 * @param
 * written_bytes	the number of bytes written into the output_buffer
 * 			during this function's call
 *
 * @return	If all the characters in the input_buffer have been
 * 		successfully converted, this function returns zero (0).
 * 		If there was not enough space in the output_buffer,
 * 		this function returns (-1).
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int convert_from_wbuffer (iconv_t *cd,
		wchar_t *input_buffer,
		char *output_buffer,
		size_t input_buffer_size,
		size_t output_buffer_size,
		size_t *written_bytes) {
	/* the variables used by the iconv */
	char *inbuf = (char *)(input_buffer);
	char *outbuf = output_buffer;
	size_t inbytesleft = input_buffer_size * sizeof (wchar_t);
	size_t outbytesleft = output_buffer_size;
	size_t iconv_retval = iconv((*cd), &inbuf, &inbytesleft,
			&outbuf, &outbytesleft);
	/*
	 * computing the number of bytes,
	 * which have just been written to the output_buffer
	 */
	(*written_bytes) = output_buffer_size - outbytesleft;
	/* if the iconv has encountered an error */
	if ((iconv_retval == (size_t)(-1)) && (errno != E2BIG)) {
		std::cerr << "iconv return value: " <<
			iconv_retval << std::endl;
		perror("convert_from_wbuffer: iconv");
		return (1); /* failure */
	/* if there was not enough space in the output_buffer */
	} else if ((iconv_retval == (size_t)(-1)) && (errno == E2BIG)) {
		/* resetting the errno */
		errno = 0;
		return (-1); /* partial success */
	} else if (iconv_retval > 0) {
		std::cerr << "convert_from_wbuffer: iconv "
			"converted " << iconv_retval << " characters\n"
			"in a nonreversible way!\n";
		return (2); /* possible failure */
	} else { /* iconv_retval == 0 */
		return (0); /* success */
	}
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
 * input_buffer	the input buffer of standard characters
 * @param
 * output_buffer	the output buffer of wide characters
 * @param
 * input_buffer_size	the number of bytes in the input buffer
 * @param
 * output_buffer_size	the number of characters in the output buffer
 * @param
 * written_characters	when this function returns, this variable will be set
 * 			to the number of characters, which has actually been
 * 			written to the specified output buffer
 *
 * @return	If the entire input buffer has been successfully converted,
 * 		this function returns zero.
 * 		If there was not enough space in the output_buffer,
 * 		this function returns (-1).
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int convert_to_wbuffer (iconv_t *cd,
		char *input_buffer,
		wchar_t *output_buffer,
		size_t input_buffer_size,
		size_t output_buffer_size,
		size_t *written_characters) {
	char *inbuf = input_buffer;
	char *outbuf = (char *)(output_buffer);
	size_t inbytesleft = input_buffer_size;
	size_t outbytesleft = output_buffer_size * sizeof (wchar_t);
	size_t outbytesleft_at_start = outbytesleft;
	size_t iconv_retval = iconv((*cd), &inbuf, &inbytesleft,
			&outbuf, &outbytesleft);
	/*
	 * now we compute the number of characters,
	 * which have just been written to the output_buffer
	 */
	(*written_characters) = (outbytesleft_at_start -
			outbytesleft) / sizeof (wchar_t);
	/* if the iconv has encountered an error */
	if ((iconv_retval == (size_t)(-1)) && (errno != E2BIG)) {
		std::cerr << "iconv return value: " <<
			iconv_retval << std::endl;
		perror("convert_to_wbuffer: iconv");
		return (1); /* failure */
	/* if there was not enough space in the output_buffer */
	} else if ((iconv_retval == (size_t)(-1)) && (errno == E2BIG)) {
		/* resetting the errno */
		errno = 0;
		return (-1); /* partial success */
	} else if (iconv_retval > 0) {
		std::cerr << "convert_to_wbuffer: iconv "
			"converted " << iconv_retval << " characters\n"
			"in a nonreversible way!\n";
		return (2); /* possible failure */
	} else { /* iconv_retval == 0 */
		return (0); /* success */
	}
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

/**
 * A function which fills the buffer of wide characters
 * with a random wide characters contained in
 * the provided probability_map pmap.
 *
 * @param
 * wbuffer	the buffer of wide characters which will be used
 * 		to store the randomly generated wide characters
 * @param
 * wbuffer_size	the desired number of wide characters
 * 		to output into the provided wbuffer
 * @param
 * pmap		the probability_map according to which
 * 		the random characters will be selected
 * @param
 * scale_factor	the number by which the generated random numbers
 * 		have to be multiplied in order to fit in the desired scale
 *
 * @return	If the desired number of wide characters
 * 		has been successfully output to the provided buffer
 * 		of wide characters, this function returns zero.
 * 		Otherwise, in case of any error,
 * 		a positive error number is returned.
 */
int fill_output_wbuffer (wchar_t *wbuffer,
		size_t wbuffer_size,
		const probability_map &pmap,
		double scale_factor) {
	/* an iterator to the provided probability_map */
	probability_map::const_iterator it = pmap.begin();
	unsigned int pseudorandom_number = 0;
	try {
		for (size_t i = 0; i < wbuffer_size; ++i) {
			/*
			 * FIXME: we suppose that the total number
			 * of input characters is not higher
			 * than the UINT_MAX, roughly.
			 */
			pseudorandom_number =
				(unsigned int)
				(rsgen::get_instance()->next());
			/*
			 * rounding and enforcing
			 * strictly positive integers
			 */
			pseudorandom_number = (unsigned int)
				((double)(pseudorandom_number) *
				scale_factor + 1.5);
			it = pmap.lower_bound(pseudorandom_number);
			if (it == pmap.end()) {
				std::cerr << "pmap.lower_bound() "
					"returned pmap.end()\n";
				return (1);
			}
			wbuffer[i] = it->second;
		}
	} catch (...) {
		std::cerr << "random character selection error!\n";
		return (2);
	}
	return (0);
}
