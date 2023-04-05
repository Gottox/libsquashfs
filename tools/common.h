/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : common.h
 */

#ifndef BIN_COMMON_H

#define BIN_COMMON_H

#ifndef _DEFAULT_SOURCE
#	define _DEFAULT_SOURCE
#endif

#include <ctype.h>
#include "../include/sqsh_archive.h"
#include "../include/sqsh_error.h"
#include "../include/sqsh_mapper.h"
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#	define VERSION "0.0.0-unknown"
#endif

static struct SqshArchive *
open_archive(const char *image_path, int *err) {
	struct SqshConfig config = {
			.source_mapper = NULL,
			.mapper_block_size = 1024 * 256,
	};
	if (sqsh_mapper_impl_curl != NULL) {
		int i;
		for (i = 0; isalnum(image_path[i]); i++) {
		}
		if (strncmp(&image_path[i], "://", 3) == 0) {
			config.source_mapper = sqsh_mapper_impl_curl;
		}
	}

	return sqsh_archive_new(image_path, &config, err);
}

#endif /* end of include guard COMMON_H */
