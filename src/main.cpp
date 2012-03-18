#include "randomc.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int print_usage (char *argv0) {
	std::cout << "Random string generator\n\n"
		"Usage:\t" << argv0 << " <alphabet> <file_size> "
		"<file_name>\n\n"
		"\t<alphabet> is a string representing "
		"the alphabet to be used\n"
		"\tFor example 'abcdefghijklmnopqrstuvwxyz' is a string\n"
		"\trepresenting the alphabet consisting of "
		"all the small English letters.\n\n"
		"\tFile <file_name> will be overwritten with "
		"the sequence of exactly\n"
		"\t<file_size> arbitrarily chosen characters "
		"from the <alphabet>.\n";
	return (0);
}

int fill_buffer (char *buffer, const char *characters, size_t buffer_length, size_t characters_length) {
	unsigned int i = 0;
	for (; i < buffer_length; ++i) {
		buffer[i] = characters[(size_t)(random()) % characters_length];
	}
	buffer[buffer_length] = '\0';
	return (0);
}

int main (int argc, char **argv) {
	size_t file_size = 0;
	size_t arg_length = 0;
	size_t alphabet_length = 0;
	size_t write_size = 0;
	size_t write_count = 0;
	size_t block_size = 8388608; /* 2^23 a.k.a. 8 Mi */
	char *endptr = NULL;
	char *buffer = NULL;
	int fd = 0;
	unsigned int i = 0;
	srandom((unsigned int)(time(NULL)));
	if (argc != 4) {
		print_usage(argv[0]);
		return (EXIT_FAILURE);
	}
	alphabet_length = strlen(argv[1]);
	if (alphabet_length == 0) {
		std::cerr << "<alphabet>: '" << argv[1] <<
			"' must contain at least one character!\n";
		return (EXIT_FAILURE);
	}
	file_size = strtoul(argv[2], &endptr, 10);
	arg_length = strlen(argv[2]);
	if (endptr != (argv[2] + arg_length)) {
		std::cerr << "<file_size>: '" << argv[2] <<
			"' is not a valid unsigned long int!\n";
		return (EXIT_FAILURE);
	}
	if (errno != 0) {
		perror("<file_size>: strtoul");
		return (EXIT_FAILURE);
	}
	if (file_size == 0) {
		std::cerr << "<file_size>: '" << argv[2] <<
			"' must be at least 1!\n";
		return (EXIT_FAILURE);
	}
	fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (fd == -1) {
		perror("<file_name>: open");
		return (EXIT_FAILURE);
	}
	try {
		buffer = new char[block_size];
	}
	catch (std::bad_alloc &) {
		std::cerr << "Memory allocation error!\n";
	}
	write_count = file_size / block_size;
	write_size = file_size % block_size;
	for (i = 0; i < write_count; ++i) {
		fill_buffer(buffer, argv[1], block_size, alphabet_length);
		if (write(fd, buffer, block_size) == (-1)) {
			perror("<file_name>: write 1");
			return (EXIT_FAILURE);
		}
	}
	if (write_size > 0) {
		fill_buffer(buffer, argv[1], write_size, alphabet_length);
		if (write(fd, buffer, write_size) == (-1)) {
			perror("<file_name>: write 2");
			return (EXIT_FAILURE);
		}
	}
	try {
		delete[] buffer;
	}
	/*
	 * We are not sure which type of exception
	 * does delete throw, if any.
	 */
	catch (std::exception &) {
		std::cerr << "Memory deallocation error!\n";
	}
	if (close(fd) == -1) {
		perror("<file_name>: close");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
