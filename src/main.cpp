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

/**
 * @file
 * The pseudorandom string generator control functions.
 * This file contains the implementation of the main function
 * and some other related functions used in rsgen.
 * which are used to handle
 * the buffering and character conversion used
 * when generating the random strings.
 */
#include "auxiliary.h"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * A function, which prints the short usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_short_usage (const char *argv0) {
	std::cout << "Usage:\t" << argv0 << "\t<distribution> -l <length> "
		"[options] filename\n\n"
		"This will generate the file 'filename' of 'length' "
		"characters\ncontaining the random characters "
		"from the specified\nprobability 'distribution'.\n\n";
	return (0);
}

/**
 * A function, which prints the help text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_help (const char *argv0) {
	print_short_usage(argv0);
	std::cout << "The probability distribution can be specified\n"
		"in the following ways:\n\n"
		"-a <alphabet>\tThe output characters will be picked\n"
		"\t\tfrom the specified alphabet\n"
		"\t\tusing the uniform distribution.\n"
		"\t\tThe 'alphabet' is a string representing\n"
		"\t\tthe alphabet to be used.\n"
		"\t\tFor example, 'abcdefghijklmnopqrstuvwxyz'\n"
		"\t\tis a string representing the alphabet\n"
		"\t\tconsisting of all the small English letters.\n"
		"-s <alphabet_size>\tThe output characters will be picked\n"
		"\t\t\tfrom the part of the Unicode starting\n"
		"\t\t\tat the character 0x0100 and spanning\n"
		"\t\t\t'alphabet_size' characters\n"
		"\t\t\tusing the uniform distribution.\n"
		"-f <ifname>\tThe output characters will be picked\n"
		"\t\tfrom the input file 'ifname'\n"
		"\t\tusing the uniform distribution.\n\n"
		"Additional options:\n\n"
		"-g <generator>\tSpecifies the desired pseudorandom\n"
		"\t\tnumber generator (PRNG) to use.\n"
		"\t\tThe available values are:\n"
		"\t\tM\tMersenne twister\n"
		"\t\tR\trandom() function\n"
		"\t\tU\t/dev/urandom system file\n"
		"\t\tThe default PRNG is the Mersenne twister.\n"
		"-i <file_encoding>\tSpecifies the character encoding\n"
		"\t\t\tof either the input alphabet string\n"
		"\t\t\tor the input file.\n"
		"\t\t\tThe default value is UTF-8.\n"
		"\t\t\tThe valid encodings are all those\n"
		"\t\t\tsupported by the iconv.\n"
		"-e <file_encoding>\tSpecifies the character encoding\n"
		"\t\t\tof the output file 'filename'.\n"
		"\t\t\tThe default value is UTF-8.\n"
		"\t\t\tThe valid encodings are all those\n"
		"\t\t\tsupported by the iconv.\n"
		"-v\t\tMakes the output more verbose.\n";
	return (0);
}

/**
 * A function, which prints the full usage text for this program.
 *
 * @param
 * argv0	the argv[0], or the command used to run this program
 *
 * @return	This function always returns zero (0).
 */
int print_usage (const char *argv0) {
	print_short_usage(argv0);
	std::cout << "For the list of available parameter values\n"
		"and additional options, run: " <<
		argv0 << " -h\n";
	return (0);
}

/* the main function */

/**
 * The main function.
 * It executes the appropriate function to generate a file
 * containing the random characters of the desired kind.
 *
 * @param
 * argc		the argument count, or the number of program arguments
 * 		(including the argv[0])
 * @param
 * argv		the argument vector itself, or an array of argument strings
 *
 * @return	If the file containing the random characters
 * 		of the desired kind has been successfully created,
 * 		this function returns EXIT_SUCCESS.
 * 		Otherwise, it returns EXIT_FAILURE.
 */
int main (int argc, char **argv) {
	size_t alphabet_size = 0;
	size_t write_size = 0;
	size_t write_count = 0;
	size_t block_size = 8388608; /* 2^23 a.k.a. 8 Mi */
	size_t input_buffer_size = 0;
	size_t output_buffer_size = block_size;
	size_t wbuffer_size = block_size;
	size_t output_length = 0;
	size_t characters_converted = 0;
	size_t wchar_t_size = sizeof(wchar_t);
	size_t bytes_to_write = 0;
	size_t bytes_read = 0;
	size_t cum_sum = 0;
	size_t last_block_characters = 0;
	size_t total_input_characters = 0;
	size_t total_bytes_written = 0;
	/*
	 * number of bytes unused in the last call
	 * to the convert_to_wbuffer function
	 */
	size_t unused_input_bytes = 0;
	char c = '\0';
	char *endptr = NULL;
	char *input_buffer = NULL;
	char *output_buffer = NULL;
	const char *input_filename = NULL;
	const char *input_encoding = "UTF-8";
	/*
	 * character encoding used in the internal text representation,
	 * it will be determined later, according to the size of the wchar_t
	 */
	const char *internal_character_encoding = NULL;
	const char *output_file_encoding = "UTF-8";
	const char *output_filename = NULL;
	wchar_t *wbuffer = NULL;
	wchar_t *output_wbuffer = NULL;
	int ifd = 0;
	int ofd = 0;
	/* The default pseudorandom number generator is the Mersenne twister. */
	int prng_type = 1;
	/* indicates whether or not we should be verbose */
	int verbose_flag = 0;
	int distribution_specification_type = 0;
	int retval = 0;
	int getopt_retval = 0;
	double scale_factor = 0;
	unsigned int i = 0;
	/* the conversion descriptor used by the iconv */
	iconv_t cd = NULL; /* iconv_t is just a typedef for void* */
	/* a std::map<wchar_t, size_t> of character occurrences */
	occurrences_map occurrences;
	/*
	 * the "probability map" used to determine the character,
	 * which will be output according to the probability
	 * of its occurrences in the input text
	 */
	probability_map pmap;
	/* parsing the command line options */
	while ((getopt_retval = getopt(argc, argv, "a:s:f:l:g:i:e:vh")) !=
			(-1)) {
		c = (char)(getopt_retval);
		switch (c) {
			case 'a':
				if (distribution_specification_type != 0) {
					std::cerr << "You can only specify "
						"one of the parameters "
						"-a -s or -f.\n\n";
					return (EXIT_FAILURE);
				}
				distribution_specification_type = 1;
				input_buffer = optarg;
				input_buffer_size = strlen(optarg);
				break;
			case 's':
				if (distribution_specification_type != 0) {
					std::cerr << "You can only specify "
						"one of the parameters "
						"-a -s or -f.\n\n";
					return (EXIT_FAILURE);
				}
				distribution_specification_type = 2;
				alphabet_size = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					std::cerr << "Unrecognized "
						"argument for the -s "
						"parameter!\n\n";
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(alphabet_size)");
					return (EXIT_FAILURE);
				}
				break;
			case 'f':
				if (distribution_specification_type != 0) {
					std::cerr << "You can only specify "
						"one of the parameters "
						"-a -s or -f.\n\n";
					return (EXIT_FAILURE);
				}
				distribution_specification_type = 3;
				input_filename = optarg;
				break;
			case 'l':
				output_length = strtoul(optarg, &endptr, 0);
				if ((*endptr) != '\0') {
					std::cerr << "Unrecognized "
						"argument for the -l "
						"parameter!\n\n";
					return (EXIT_FAILURE);
				}
				if (errno != 0) {
					perror("strtoul(output_length)");
					return (EXIT_FAILURE);
				}
				break;
			case 'g':
				if (optarg[0] == 'M') {
					prng_type = 1;
				} else if (optarg[0] == 'R') {
					prng_type = 2;
				} else if (optarg[0] == 'U') {
					prng_type = 3;
				} else {
					std::cerr << "Unrecognized "
						"argument for the -g "
						"parameter!\n\n";
					return (EXIT_FAILURE);
				}
				break;
			case 'i':
				input_encoding = optarg;
				break;
			case 'e':
				output_file_encoding = optarg;
				break;
			case 'v':
				verbose_flag = 1;
				break;
			case 'h':
				print_help(argv[0]);
				return (EXIT_SUCCESS);
			case '?':
				return (EXIT_FAILURE);
		}
	}
	if (distribution_specification_type == 0) {
		std::cerr << "At least one of the parameters -a, -s or -f\n"
			"describing the probability distribution "
			"must be specified!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	} else if (distribution_specification_type == 1) {
		/* FIXME: Is it true that this can never happen? */
		if (input_buffer_size == 0) {
			std::cerr << "<alphabet>: '" << input_buffer <<
				"' must contain at least one character!\n";
			print_usage(argv[0]);
			return (EXIT_FAILURE);
		}
	} else if (distribution_specification_type == 2) {
		if (alphabet_size == 0) {
			std::cerr << "<alphabet_size> must be "
				"strictly positive!\n";
			print_usage(argv[0]);
			return (EXIT_FAILURE);
		}
	}
	if (output_length == 0) {
		std::cerr << "The parameter -l is mandatory\n"
			"and it ought to be positive!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	if (optind == argc) {
		std::cerr << "Missing the 'filename' parameter!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	output_filename = argv[optind];
	if (optind + 1 < argc) {
		std::cerr << "Too many parameters!\n\n";
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	/* command line options parsing complete */
	if (wchar_t_size == 1) {
		std::clog << "Warning: The size of the data type wchar_t\n"
			"is only a single byte!\n\n";
		/*
		 * we can not use Unicode, so by default we stick
		 * to the basic ASCII encoding
		 */
		internal_character_encoding = "ASCII";
	} else if ((wchar_t_size > 1) && (wchar_t_size < 4)) {
		/*
		 * We can use limited Unicode (Basic Multilingual Plane,
		 * or BMP only). We prefer UCS-2 to UTF-16, because we would
		 * not like to deal with the byte order marks (BOM).
		 */
		/* we suppose we are on the little endian architecture */
		internal_character_encoding = "UCS-2LE";
	} else { /* wchar_t_size >= 4 */
		/*
		 * We can use full Unicode (all the code points). We prefer
		 * UCS-4 to UTF-32, because we would not like to deal
		 * with the byte order marks (BOM).
		 */
		/* again, we suppose the little endian architecture */
		internal_character_encoding = "UCS-4LE";
	}
	std::cout << "Random string generator (rsgen)" <<
		std::endl << std::endl;
	if (verbose_flag != 0) {
		std::cout << "Selected pseudorandom number generator: ";
		switch (prng_type) {
			case 1 : /* Mersenne twister */
				std::cout << "Mersenne twister\n";
				break;
			case 2 : /* the random() function */
				std::cout << "random() function\n";
				break;
			case 3 : /* the /dev/urandom system file */
				std::cout << "/dev/urandom system file\n";
				break;
			default:
				std::cout << "unknown (prng_type == " <<
					prng_type << ")\n";
		}
		std::cout << "Input character encoding: '" <<
			input_encoding << "'\n";
		std::cout << "Internal character encoding: '" <<
			internal_character_encoding << "'\n\n";
		std::cout << "Size of wchar_t data type: " <<
			wchar_t_size << " bytes\n";
	}
	/* if the user supplied the alphabet string */
	if (distribution_specification_type == 1) {
		std::cout << "Reading the input alphabet.\n";
		/* we create the desired conversion descriptor */
		if ((cd = iconv_open(internal_character_encoding,
					input_encoding)) == (iconv_t)(-1)) {
			perror("iconv_open 1");
			return (EXIT_FAILURE);
		}
		wbuffer_size = block_size;
		try {
			wbuffer = new wchar_t[wbuffer_size];
		} catch (std::bad_alloc &) {
			std::cerr << "wbuffer allocation error!\n";
			return (EXIT_FAILURE);
		}
		if (convert_to_wbuffer(&cd, input_buffer, wbuffer,
					input_buffer_size, wbuffer_size,
					&unused_input_bytes,
					&characters_converted) > 0) {
			std::cerr << "Character conversion error!\n";
			return (EXIT_FAILURE);
		}
		if (add_character_occurrences(occurrences,
					wbuffer, characters_converted) > 0) {
			std::cerr << "Could not determine the numbers of "
				"occurrences\nof the individual characters!\n";
			return (EXIT_FAILURE);
		}
		if (iconv_close(cd) == (-1)) {
			perror("iconv_close 1");
			return (EXIT_FAILURE);
		}
		total_input_characters = characters_converted;
		std::cout << "The input alphabet has been successfully read!\n";
	/* if the user supplied the size of the alphabet */
	} else if (distribution_specification_type == 2) {
		std::cout << "Generating the input alphabet of size " <<
			alphabet_size << ".\n";
		try {
			wbuffer = new wchar_t[alphabet_size];
		} catch (std::bad_alloc &) {
			std::cerr << "wbuffer allocation error!\n";
			return (EXIT_FAILURE);
		}
		/*
		 * Filling in the Unicode characters
		 * starting at the position 0x0100.
		 */
		for (i = 0; i < alphabet_size; ++i) {
			wbuffer[i] = (wchar_t)(0x0100 + i);
		}
		if (add_character_occurrences(occurrences,
					wbuffer, alphabet_size) > 0) {
			std::cerr << "Could not determine the numbers of "
				"occurrences\nof the individual characters!\n";
			return (EXIT_FAILURE);
		}
		total_input_characters = alphabet_size;
		std::cout << "The input alphabet has been "
			"successfully generated!\n";
	/* if the user supplied the name of the input file */
	} else if (distribution_specification_type == 3) {
		std::cout << "Reading the input file '" <<
			input_filename << "'.\n";
		/* we try to open the input file for reading */
		ifd = open(input_filename, O_RDONLY);
		if (ifd == (-1)) {
			perror("input_filename: open");
			return (EXIT_FAILURE);
		}
		/* we create the desired conversion descriptor */
		if ((cd = iconv_open(internal_character_encoding,
					input_encoding)) == (iconv_t)(-1)) {
			perror("iconv_open 2");
			return (EXIT_FAILURE);
		}
		input_buffer_size = block_size;
		try {
			input_buffer = new char[input_buffer_size];
			wbuffer = new wchar_t[input_buffer_size];
		} catch (std::bad_alloc &) {
			std::cerr << "input_buffer or wbuffer "
				"allocation error!\n";
			return (EXIT_FAILURE);
		}
		/* here, total_input_characters should be equal to 0 */
		do {
			if ((retval = text_file_read_buffer(ifd,
					input_buffer + unused_input_bytes,
					input_buffer_size - unused_input_bytes,
					&bytes_read)) > 0) {
				std::cerr <<
					"Could not read the input file!\n";
				return (EXIT_FAILURE);
			}
			if (convert_to_wbuffer(&cd, input_buffer, wbuffer,
					bytes_read + unused_input_bytes,
					wbuffer_size, &unused_input_bytes,
					&characters_converted) > 0) {
				std::cerr << "Character conversion error!\n";
				return (EXIT_FAILURE);
			}
			if (add_character_occurrences(occurrences,
					wbuffer, characters_converted) > 0) {
				std::cerr << "Could not determine "
					"the numbers of occurrences\n"
					"of the individual characters!\n";
				return (EXIT_FAILURE);
			}
			total_input_characters += characters_converted;
		} while (retval == 0);
		if (retval != (-1)) {
			std::cerr << "Error: The last call to the function\n"
				"text_file_read_buffer"
				" has not been successful.\n";
			return (EXIT_FAILURE);
		}
		if (unused_input_bytes != (size_t)(0)) {
			std::cerr << "Error: The last call to the function\n"
				"convert_to_wbuffer"
				" did not convert all the provided bytes.\n";
			return (EXIT_FAILURE);
		}
		delete[] input_buffer;
		if (iconv_close(cd) == (-1)) {
			perror("iconv_close 2");
			return (EXIT_FAILURE);
		}
		if (close(ifd) == -1) {
			perror("input_filename: close");
			return (EXIT_FAILURE);
		}
		std::cout << "Input file has been successfully read!\n";
	}
	delete[] wbuffer;
	cum_sum = 0;
	for (occurrences_map::iterator it = occurrences.begin();
			it != occurrences.end(); ++it) {
		cum_sum += it->second;
		pmap[cum_sum] = it->first;
	}
	if (total_input_characters != cum_sum) {
		std::cerr << "Something went wrong,\nbecause total number "
			"of input characters (" << total_input_characters
			<< ")\nis not equal to the cumulative "
			"sum of the occurrences\nof all the characters (" <<
			cum_sum << ").\n";
		return (EXIT_FAILURE);
	}
	if (verbose_flag != 0) {
		std::cout << "Total alphabet size: " << pmap.size() << "\n";
	}
	/* initializing the pseudorandom number generator */
	rsgen::instance(prng_type);
	ofd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP |
			S_IROTH | S_IWOTH);
	if (ofd == -1) {
		perror("output_filename: open");
		return (EXIT_FAILURE);
	}
	if ((cd = iconv_open(output_file_encoding,
			internal_character_encoding)) == (iconv_t)(-1)) {
		perror("iconv_open 3");
		return (EXIT_FAILURE);
	}
	try {
		output_wbuffer = new wchar_t[block_size];
	} catch (std::bad_alloc &) {
		std::cerr << "output_wbuffer allocation error!\n";
		return (EXIT_FAILURE);
	}
	/*
	 * we suppose that the maximum number of bytes that encode
	 * a single UTF-8 character can never exceed 6
	 */
	output_buffer_size = block_size * 6;
	try {
		output_buffer = new char[output_buffer_size];
	} catch (std::bad_alloc &) {
		std::cerr << "output_buffer allocation error!\n";
		return (EXIT_FAILURE);
	}
	write_count = output_length / block_size;
	write_size = output_length % block_size;
	last_block_characters = write_size;
	/*
	 * we have to decrease the size of the interval of random numbers
	 * by one, because we later add 1 to every generated random number
	 */
	scale_factor = (double)(total_input_characters - 1) /
		(double)(UINT_MAX);
	/* here, we suppose that total_bytes_written == 0 */
	std::cout << "\nGenerating the random file '" <<
		output_filename << "'\n";
	std::cout << "Output file encoding: '" <<
		output_file_encoding << "'\n";
	for (i = 0; i < write_count; ++i) {
		if (fill_output_wbuffer(output_wbuffer, block_size,
					pmap, scale_factor) != 0) {
			return (EXIT_FAILURE);
		}
		if (convert_from_wbuffer(&cd, output_wbuffer, output_buffer,
				block_size, output_buffer_size,
				&bytes_to_write) != 0) {
			return (EXIT_FAILURE);
		}
		if (write(ofd, output_buffer, bytes_to_write) == (-1)) {
			perror("output_filename: write 1");
			return (EXIT_FAILURE);
		}
		total_bytes_written += bytes_to_write;
	}
	if (write_size > 0) {
		if (fill_output_wbuffer(output_wbuffer, last_block_characters,
					pmap, scale_factor) != 0) {
			return (EXIT_FAILURE);
		}
		if (convert_from_wbuffer(&cd, output_wbuffer, output_buffer,
				last_block_characters,
				output_buffer_size,
				&bytes_to_write) != 0) {
			return (EXIT_FAILURE);
		}
		if (write(ofd, output_buffer, bytes_to_write) == (-1)) {
			perror("output_filename: write 2");
			return (EXIT_FAILURE);
		}
		total_bytes_written += bytes_to_write;
	}
	std::cout << "Successfully written " << output_length <<
		" characters (" << total_bytes_written << " bytes)\n";
	delete[] output_buffer;
	delete[] output_wbuffer;
	if (iconv_close(cd) == (-1)) {
		perror("iconv_close 3");
		return (EXIT_FAILURE);
	}
	if (close(ofd) == -1) {
		perror("output_filename: close");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
