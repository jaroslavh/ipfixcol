/**
 * \file utils.c
 * \author Michal Kozubik <kozubik@cesnet.cz>
 * \brief Functions unrelated to IPFIX data parsing
 *
 * Copyright (C) 2015 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <ipfixcol.h>

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>

#define NUMBER_OF_INPUT_FILES 100

static const char *msg_module = "utils";

/**
 * \brief determine whether string matches regexp or not
 *
 * \return 1 if string matches regexp, 0 otherwise */
static int regexp_asterisk(char *regexp, char *string)
{
	static int asterisk = '*';
	char *asterisk_pos;
	char *aux_regexp;
	char *saveptr;
	char *token;
	int ok;             /* 1 if string matches regexp, 0 otherwise */

	if ((regexp == NULL) || (string == NULL)) {
		return 0;
	}

	if ((asterisk_pos = strchr(regexp, asterisk)) == NULL) {
		/* this string doesn't contain asterisk... */
		if (!strcmp(regexp, string)) {
			/* we've found a match! */
			return 1;
		} else {
			return 0;
		}
	}

	/* make copy of original string */
	aux_regexp = (char *) malloc(strlen(regexp) + 1);
	if (aux_regexp == NULL) {
		MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
		return -1;
	}

	strncpy_safe(aux_regexp, regexp, strlen(regexp) + 1);

	int pos = 1; /* we assume that asterisk is in the middle of the string */
	if (aux_regexp[0] == asterisk) {
		/* asterisk on the beginning */
		pos = 0;
	}
	if (aux_regexp[strlen(aux_regexp)-1] == asterisk) {
		/* asterisk on the end of the string */
		pos = 2;
	}
	if (!strcmp(aux_regexp, "*")) {
		/* there is nothing but asterisk */
		pos = -1;
	}

	token = strtok_r(aux_regexp, "*", &saveptr);

	ok = 0;
	switch (pos) {
	case (-1):
		/* there is nothing but asterisk, so it matches. best scenario :) */
		ok = 1;
		break;
	case (0):
		/* asterisk is on the beginning of the string */
		if (!strncmp(token, string+(strlen(string)-strlen(token)), strlen(token))) {
			ok = 1;
		}
		break;

	case (1):
		/* asterisk is in the middle of the string */
		if (!strncmp(token, string, strlen(token))) {
			token = strtok_r(NULL, "*", &saveptr);
			if (!strncmp(token, string+(strlen(string)-strlen(token)), strlen(token))) {
				ok = 1;
			}
		}
		break;

	case (2):
		/* asterisk is on the end of the string */
		if (!strncmp(token, string, strlen(token))) {
			ok = 1;
		}
		break;
	}

	free(aux_regexp);
	return ok;
}

/**
 * \brief Compare function for qsort
 */
static int compare(const void *a, const void *b)
{
	return strcmp(*(const char **) a, *(const char **) b);
}


char **utils_files_from_path(char *path)
{
	DIR *dir = NULL;
	char **input_files = NULL;
	char *dirname = NULL, *filename = NULL;
	struct dirent *entry = NULL, *result = NULL;
	struct stat st;

	int ret = 0, array_length = 0, inputf_index = 0;

	dirname  = utils_dir_from_path(path);
	if (!dirname) {
		goto err_file;
	}

	filename = basename(path);
	if (!filename) {
		goto err_file;
	}

	dir = opendir(dirname);
	if (dir == NULL) {
		MSG_ERROR(msg_module, "Unable to open input file(s); directory cannot be found (%s)\n", dirname);
		free(dirname);
		return NULL;
	}

	int len = offsetof(struct dirent, d_name) +
			pathconf(dirname, _PC_NAME_MAX) + 1;

	entry = (struct dirent *) malloc(len);
	if (entry == NULL) {
		MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
		goto err_file;
	}

	array_length = NUMBER_OF_INPUT_FILES;
	input_files = (char **) calloc(array_length, sizeof(char *));
	if (input_files == NULL) {
		MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
		goto err_file;
	}

	do {
		ret = readdir_r(dir, entry, &result);
		if (ret != 0) {
			MSG_ERROR(msg_module, "Error while reading directory '%s'\n", dirname);
			goto err_file;
		}

		if (result == NULL) {
			/* no more files in directory */
			break;
		}

		if ((!strcmp(".", entry->d_name)) || (!strcmp("..", entry->d_name))) {
			continue;
		}

		/* check whether this filename matches given regexp */
		ret = regexp_asterisk(filename, entry->d_name);
		if (ret == 1) {
			/* this file matches */
			if (inputf_index >= array_length - 1) {
				input_files = realloc(input_files, array_length * 2);
				if (input_files == NULL) {
					MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
					goto err_file;
				}
				array_length *= 2;
			}

			input_files[inputf_index] = (char *) malloc(strlen(entry->d_name) + strlen(dirname) + 2); /* 2 because of "/" and NULL*/
			if (input_files[inputf_index] == NULL) {
				MSG_ERROR(msg_module, "Memory allocation failed (%s:%d)", __FILE__, __LINE__);
				goto err_file;
			}

			/* create path+filename string */
			sprintf(input_files[inputf_index], "%s/%s", dirname, entry->d_name);

			/* check whether input file is a directory */
			if (stat(input_files[inputf_index], &st) == -1) {
				MSG_WARNING(msg_module, "");
				free(input_files[inputf_index]);
				input_files[inputf_index] = NULL;
				MSG_WARNING(msg_module, "Could not determine stats for '%s'", entry->d_name);
				continue;
			}

			if (S_ISDIR(st.st_mode)) {
				free(input_files[inputf_index]);
				input_files[inputf_index] = NULL;
				MSG_WARNING(msg_module, "Input file '%s' is a directory; skipping...", entry->d_name);
				continue;
			}

			inputf_index += 1;
		}
	} while (result);

	/* Sort file names - we need them ordered for tests */
	qsort(input_files, inputf_index, sizeof(const char *), compare);

	input_files[inputf_index] = NULL;

	closedir(dir);
	free(entry);
	free(dirname);

	return input_files;

err_file:
	if (dirname) {
		free(dirname);
	}

	if (dir) {
		closedir(dir);
	}

	if (input_files) {
		free(input_files);
	}

	if (entry) {
		free(entry);
	}

	return NULL;
}

char *utils_dir_from_path(char *path)
{
	char *dir = strdup(path);
	dir = dirname(dir);

	return dir;
}

/**
 * \brief Version of strncpy that ensures null-termination.
 */
char *strncpy_safe(char *destination, const char *source, size_t num)
{
	strncpy(destination, source, num);

	// Ensure null-termination
	destination[num - 1] = '\0';

	return destination;
}

/**
 * \brief Version of strtol with proper error-handling.
 *
 * \return Converted integer value of the supplied String, INT_MAX otherwise.
 */
int strtoi(const char* str, int base)
{
	char *end;
	errno = 0;

	if (str == NULL) {
		return INT_MAX;
	}

	const long ret_long = strtol(str, &end, base);
	int ret_int;

	if (end == str) { // String does not feature a valid number
		ret_int = INT_MAX;
	} else if ((ret_long <= INT_MIN || ret_long >= INT_MAX) && errno == ERANGE) { // Number is out of range
		ret_int = INT_MAX;
	} else {
		ret_int = (int) ret_long;
	}

	return ret_int;
}

/**
 * \brief Formats the path string according to the format specification
 * \param[in] original Path with special character sequences
 * \return On success returns a pointer to the newly created path. Otherwise
 *   returns a non-zero value.
 */
char *utils_path_preprocessor(const char *original)
{
	char tmp[PATH_MAX];
	char *last_path = tmp;
	char *perc_sign;
	char *new_str; //new path

	const size_t err_len = 128;
	char err_buff[err_len];
	err_buff[0] = '\0';

	if (original == NULL) {
		return NULL;
	}

	if (strlen(original) > PATH_MAX - 1) {
		strerror_r(ENAMETOOLONG, err_buff, err_len);
		MSG_ERROR(msg_module, "Path preprocessor failed (%s \"%s\")",
			err_buff, original);
		return NULL;
	}

	strcpy(tmp, original);

	new_str = calloc(PATH_MAX, sizeof (char));
	if (new_str == NULL) {
		MSG_ERROR(msg_module, "Unable to allocate memory (%s:%d)",
			__FILE__, __LINE__);
		return NULL;
	}

	/* last_path == tmp */
	perc_sign = strchr(last_path, '%'); //find first percent sign
	while (perc_sign != NULL) {
		*perc_sign = '\0';
		strcat(new_str, last_path); //copy original path till percent sign

		perc_sign++; //move pointer to escaped character
		switch (*perc_sign) {
		case 'h':
			errno = 0;
			gethostname(new_str + strlen(new_str), PATH_MAX - strlen(new_str));
			if (errno != 0) {
				strerror_r(ENAMETOOLONG, err_buff, err_len);
				MSG_ERROR(msg_module, "Path preprocessor failed (%s \"%s\")",
					err_buff, original);
				free(new_str);
				return NULL;
			}
			break;
		default:
			MSG_ERROR(msg_module, "Path preprocessor failed (Unknown escape "
				"sequence \"%s\"", original);
			free(new_str);
			return NULL;
		}

		last_path = perc_sign + 1; //ptr to next regular character
		perc_sign = strchr(last_path, '%');
	}

	strcat(new_str, last_path); //copy rest of the original path
	return new_str;
}
