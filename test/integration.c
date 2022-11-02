/******************************************************************************
 *                                                                            *
 * Copyright (c) 2022, Enno Boland <g@s01.de>                                 *
 *                                                                            *
 * Redistribution and use in source and binary forms, with or without         *
 * modification, are permitted provided that the following conditions are     *
 * met:                                                                       *
 *                                                                            *
 * * Redistributions of source code must retain the above copyright notice,   *
 *   this list of conditions and the following disclaimer.                    *
 * * Redistributions in binary form must reproduce the above copyright        *
 *   notice, this list of conditions and the following disclaimer in the      *
 *   documentation and/or other materials provided with the distribution.     *
 *                                                                            *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS    *
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,  *
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR          *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,      *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,        *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR         *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF     *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING       *
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               *
 *                                                                            *
 ******************************************************************************/

/**
 * @author       Enno Boland (mail@eboland.de)
 * @file         integration.c
 */

#include "../src/context/file_context.h"
#include "../src/context/inode_context.h"
#include "../src/context/superblock_context.h"
#include "../src/error.h"
#include "../src/iterator/directory_iterator.h"
#include "../src/iterator/xattr_iterator.h"
#include "../src/sqsh.h"
#include "../src/table/xattr_table.h"
#include "common.h"
#include "test.h"
#include <squashfs_image.h>
#include <stdint.h>

static void
sqsh_empty() {
	int rv;
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = 0,
	};
	rv = sqsh_init(&sqsh, NULL, &config);
	assert(rv == -SQSH_ERROR_SUPERBLOCK_TOO_SMALL);
}

static void
sqsh_get_nonexistant() {
	int rv;
	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};

	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "/nonexistant");
	assert(rv < 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_ls() {
	int rv;
	char *name;
	struct SqshInodeContext inode = {0};
	struct SqshDirectoryIterator iter = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_root(&inode, &sqsh);
	assert(rv == 0);

	rv = sqsh_directory_iterator_init(&iter, &inode);
	assert(rv == 0);

	rv = sqsh_directory_iterator_next(&iter);
	assert(rv > 0);
	rv = sqsh_directory_iterator_name_dup(&iter, &name);
	assert(rv == 1);
	assert(strcmp("a", name) == 0);
	free(name);

	rv = sqsh_directory_iterator_next(&iter);
	assert(rv >= 0);
	rv = sqsh_directory_iterator_name_dup(&iter, &name);
	assert(rv == 1);
	assert(strcmp("b", name) == 0);
	free(name);

	rv = sqsh_directory_iterator_next(&iter);
	assert(rv >= 0);
	rv = sqsh_directory_iterator_name_dup(&iter, &name);
	assert(rv == 9);
	assert(strcmp("large_dir", name) == 0);
	free(name);

	rv = sqsh_directory_iterator_next(&iter);
	// End of file list
	assert(rv == 0);

	rv = sqsh_directory_iterator_cleanup(&iter);
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_cat_fragment() {
	int rv;
	const uint8_t *data;
	size_t size;
	struct SqshInodeContext inode = {0};
	struct SqshFileContext file = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "a");
	assert(rv == 0);

	rv = sqsh_file_init(&file, &inode);
	assert(rv == 0);

	size = sqsh_inode_file_size(&inode);
	assert(size == 2);

	rv = sqsh_file_read(&file, size);
	assert(rv == 0);

	data = sqsh_file_data(&file);
	assert(memcmp(data, "a\n", size) == 0);

	rv = sqsh_file_cleanup(&file);
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_cat_datablock_and_fragment() {
	int rv;
	const uint8_t *data;
	size_t size;
	struct SqshInodeContext inode = {0};
	struct SqshFileContext file = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "b");
	assert(rv == 0);

	rv = sqsh_file_init(&file, &inode);
	assert(rv == 0);

	size = sqsh_inode_file_size(&inode);
	assert(size == 1050000);

	rv = sqsh_file_read(&file, size);
	assert(rv == 0);
	assert(size == sqsh_file_size(&file));

	data = sqsh_file_data(&file);
	for (sqsh_index_t i = 0; i < size; i++) {
		assert(data[i] == 'b');
	}

	rv = sqsh_file_cleanup(&file);
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_cat_size_overflow() {
	int rv;
	const uint8_t *data;
	size_t size;
	struct SqshInodeContext inode = {0};
	struct SqshFileContext file = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "b");
	assert(rv == 0);

	rv = sqsh_file_init(&file, &inode);
	assert(rv == 0);
	size = sqsh_inode_file_size(&inode);
	assert(size == 1050000);

	rv = sqsh_file_read(&file, size + 4096);
	assert(rv != 0); // TODO: check for correct error code

	assert(sqsh_file_size(&file) == size);

	data = sqsh_file_data(&file);
	for (sqsh_index_t i = 0; i < size; i++) {
		assert(data[i] == 'b');
	}

	rv = sqsh_file_cleanup(&file);
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_test_uid_and_gid() {
	int rv;
	uint32_t uid, gid;
	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_root(&inode, &sqsh);
	assert(rv == 0);

	uid = sqsh_inode_uid(&inode);
	assert(uid == 2020);
	gid = sqsh_inode_gid(&inode);
	assert(gid == 202020);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_test_extended_dir() {
	int rv;
	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "/large_dir/999");
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
sqsh_test_xattr() {
	const char *expected_value = "1234567891234567891234567890001234567890";
	int rv;
	char *name, *value;
	struct SqshInodeContext inode = {0};
	struct SqshInodeContext entry_inode = {0};
	struct SqshDirectoryIterator dir_iter = {0};
	struct SqshXattrIterator xattr_iter = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(squash_image),
	};
	rv = sqsh_init(&sqsh, (char *)squash_image, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_root(&inode, &sqsh);
	assert(rv == 0);

	rv = sqsh_inode_xattr_iterator(&inode, &xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_next(&xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_cleanup(&xattr_iter);
	assert(rv == 0);

	rv = sqsh_directory_iterator_init(&dir_iter, &inode);
	assert(rv == 0);

	rv = sqsh_directory_iterator_next(&dir_iter);
	assert(rv > 0);
	rv = sqsh_directory_iterator_name_dup(&dir_iter, &name);
	assert(rv == 1);
	assert(strcmp("a", name) == 0);
	free(name);
	rv = sqsh_directory_iterator_inode_load(&dir_iter, &entry_inode);
	assert(rv == 0);
	rv = sqsh_inode_xattr_iterator(&entry_inode, &xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_next(&xattr_iter);
	assert(rv > 0);
	assert(sqsh_xattr_iterator_is_indirect(&xattr_iter) == false);
	rv = sqsh_xattr_iterator_fullname_dup(&xattr_iter, &name);
	assert(rv == 8);
	assert(strcmp("user.foo", name) == 0);
	free(name);
	rv = sqsh_xattr_iterator_value_dup(&xattr_iter, &value);
	assert(rv == (int)strlen(expected_value));
	assert(strcmp(expected_value, value) == 0);
	free(value);
	rv = sqsh_xattr_iterator_next(&xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_cleanup(&xattr_iter);
	assert(rv == 0);
	rv = sqsh_inode_cleanup(&entry_inode);
	assert(rv == 0);

	rv = sqsh_directory_iterator_next(&dir_iter);
	assert(rv >= 0);
	rv = sqsh_directory_iterator_name_dup(&dir_iter, &name);
	assert(rv == 1);
	assert(strcmp("b", name) == 0);
	free(name);
	rv = sqsh_directory_iterator_inode_load(&dir_iter, &entry_inode);
	assert(rv == 0);
	rv = sqsh_inode_xattr_iterator(&entry_inode, &xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_next(&xattr_iter);
	assert(rv > 0);
	assert(sqsh_xattr_iterator_is_indirect(&xattr_iter) == true);
	rv = sqsh_xattr_iterator_fullname_dup(&xattr_iter, &name);
	assert(rv == 8);
	assert(strcmp("user.bar", name) == 0);
	free(name);
	rv = sqsh_xattr_iterator_value_dup(&xattr_iter, &value);
	assert(rv == (int)strlen(expected_value));
	assert(strcmp(expected_value, value) == 0);
	free(value);
	rv = sqsh_xattr_iterator_next(&xattr_iter);
	assert(rv == 0);
	rv = sqsh_xattr_iterator_cleanup(&xattr_iter);
	assert(rv == 0);
	rv = sqsh_inode_cleanup(&entry_inode);
	assert(rv == 0);

	rv = sqsh_directory_iterator_cleanup(&dir_iter);
	assert(rv == 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
fuzz_crash_1() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x3,  0x0,  0x0,  0x0,  0x96, 0x97, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x3e, 0x1,  0x0,  0x0,  0x0,  0x0,  0x3,  0x0,
			0x0,  0x64, 0x1,  0x1d, 0x0,  0x0,  0x96, 0x97, 0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x3e, 0x1,  0x0,  0x0,  0x0,  0x0,  0x0,  0x32, 0x62, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x32, 0x0,  0x0,
			0x0,  0x0,  0x0,  0x60, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
			0x1,  0x0,  0x62, 0x62, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x36,
			0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
			0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
			0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62,
			0x62, 0x62, 0x62, 0x62, 0x62, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x60, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x1,  0x0,
			0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x62, 0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0xfa, 0xff, 0xff, 0xff, 0xff, 0xff,
			0xff, 0xff, 0xff, 0xff, 0xff, 0x36, 0x62, 0x62, 0x62, 0x62, 0x62,
			0x62, 0x62, 0x29, 0x62, 0x62, 0x62, 0x62, 0xff, 0xff, 0x62, 0x62};

	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "");
	assert(rv < 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
fuzz_crash_2() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x23, 0x0,  0x0,  0x0,  0x96, 0x97, 0x68,
			0x61, 0x1,  0x0,  0x2,  0x0,  0x1,  0x0,  0x10, 0x0,  0x1,  0x0,
			0x11, 0x0,  0xcb, 0x1,  0x1,  0x0,  0x4,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x7,  0x0,  0x0,  0x0,  0x0,  0x0,  0x64, 0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x62, 0x62, 0x62, 0x62,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x60, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x62, 0x62,
			0x62, 0x62, 0x62, 0x11, 0x0,  0xcb, 0x1,  0x1,  0x0,  0x4,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x60, 0x0,  0x0,  0x0,  0x62,
			0x62, 0x62, 0x0,  0x2,
	};

	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "");
	assert(rv < 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
fuzz_crash_3() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x23, 0x0, 0x0,  0x0,  0x96, 0x97, 0x68,
			0x61, 0x1,  0x0,  0x2,  0x0,  0x1, 0x1,  0x10, 0x0,  0x5,  0x0,
			0x11, 0x0,  0xcb, 0x1,  0x1,  0x0, 0x4,  0x0,  0x0,  0x0,  0x0,
			0xb9, 0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x64, 0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x76, 0x0, 0x0,  0x0,  0x62, 0x62, 0x62,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x40, 0x0,  0x0,  0x60, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x1,  0x1d,
			0x73, 0x71, 0x73, 0x23, 0x0,  0x0, 0x0,  0x96, 0x97, 0x68, 0x61,
			0x1,  0x0,  0x2,  0x0,  0x1,  0x1, 0x0,  0x0,  0x2,  0x0,  0x11,
			0x0,  0xcb, 0x74, 0x71, 0x0,  0x0, 0x74, 0x71, 0x0,  0x0,  0x68,
			0x61, 0x1,  0x0,  0x0,  0x2,  0x2,
	};

	struct SqshInodeContext inode = {0};
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);

	rv = sqsh_inode_init_by_path(&inode, &sqsh, "");
	assert(rv < 0);

	rv = sqsh_inode_cleanup(&inode);
	assert(rv == 0);

	rv = sqsh_cleanup(&sqsh);
	assert(rv == 0);
}

static void
fuzz_crash_4() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0xa,  0xf8, 0x0,  0x0,  0x0,  0x0,  0xb1,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x30, 0x0,  0x8c, 0x0,  0x0,  0x0,
			0x0,  0x0,  0x6,  0x0,  0x0,  0x0,  0x0,  0x1,  0x68, 0x73, 0xf4,
			0xa,  0x41, 0x0,  0x0,  0x0,  0x0,  0x0,  0xf1, 0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0xf1, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x84, 0x0,  0x0,  0x0,  0xad, 0x0,  0x0,  0x0,  0x71, 0x1,
			0x0,  0x0,  0x1,  0x0,  0x0,  0x0,  0x62, 0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x2,  0x1,  0x0,  0x0,  0x68, 0x73,
			0x71, 0x73, 0xa,  0x0,  0xb1, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x1,  0x68, 0x73, 0x71, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
			0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
			0xaf, 0x73, 0xa,  0xff, 0xff, 0x0,  0x23, 0x0,  0x62, 0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x2,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x1,  0xb,  0x0,  0x2,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x68, 0x0,  0x0,  0x31, 0x0,  0x0,  0x2,  0x73,
			0x1d, 0x1d, 0x1,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x62,
			0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x14, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d,
			0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x71, 0x3b, 0x3b, 0x11, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0xa,  0x0,  0x0,  0x0,  0x0,  0x0,
			0xb1, 0x0,  0x0,  0x0,  0x0,  0x0,  0x30, 0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
	};

	struct SqshTable *id_table = NULL;
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);
	rv = sqsh_id_table(&sqsh, &id_table);
	assert(rv == -SQSH_ERROR_SIZE_MISSMATCH);
	sqsh_cleanup(&sqsh);
}

static void
fuzz_crash_5() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb1,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x8c, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00,
			0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x01, 0x00,
			0x00, 0x68, 0x73, 0x71, 0x73, 0x0a, 0x00, 0xb1, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x01, 0x68, 0x73, 0x71, 0xaf, 0xaf, 0xaf,
			0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
			0xaf, 0xaf, 0xaf, 0xaf, 0x73, 0x0a, 0xff, 0xff, 0x00, 0x23, 0x00,
			0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x00, 0x02, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
			0x73, 0x1d, 0x1d, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x62, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x14, 0x1d, 0x1d, 0x1d,
			0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x71, 0x3b, 0x3b,
			0x11, 0x00, 0x00, 0x00, 0x00, 0xff, 0xfd, 0x00, 0x00, 0x00, 0x00,
			0x68, 0x73, 0x71, 0x73, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb1,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x02, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00,
	};

	struct SqshTable *id_table = NULL;
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);
	rv = sqsh_id_table(&sqsh, &id_table);
	assert(rv == -SQSH_ERROR_SIZE_MISSMATCH);
	sqsh_cleanup(&sqsh);
}

static void
fuzz_crash_6() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x0,  0x0, 0x0, 0x0, 0x80, 0x0,  0x0, 0x1,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
			0x0,  0x0,  0x0,  0x69, 0xfb, 0x0, 0x0, 0x0, 0x0,  0x10, 0x0, 0x0,
			0x0,  0x0,  0xf7, 0x0,  0x60, 0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
			0x0,  0x1,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0, 0x0, 0x0, 0x0,  0x0,  0x0, 0xa,
	};

	struct SqshTable *id_table = NULL;
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);
	rv = sqsh_id_table(&sqsh, &id_table);
	assert(rv == -SQSH_ERROR_SIZE_MISSMATCH);
	sqsh_cleanup(&sqsh);
}

static void
fuzz_crash_7() {
	int rv;
	static const uint8_t input[] = {
			0x68, 0x73, 0x71, 0x73, 0x97, 0x97, 0x97, 0x97, 0x97, 0x97, 0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x7e, 0xa6, 0xa6, 0xa6,
			0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,
			0x0,  0x97, 0x97, 0x97, 0x97, 0x74, 0x97, 0x97,
	};

	struct SqshTable *id_table = NULL;
	struct Sqsh sqsh = {0};
	const struct SqshConfig config = {
			.source_type = SQSH_SOURCE_TYPE_MEMORY,
			.source_size = sizeof(input),
	};
	rv = sqsh_init(&sqsh, input, &config);
	assert(rv == 0);
	rv = sqsh_id_table(&sqsh, &id_table);
	assert(rv == -SQSH_ERROR_SIZE_MISSMATCH);
	sqsh_cleanup(&sqsh);
}

DEFINE
TEST(sqsh_empty);
TEST(sqsh_ls);
TEST(sqsh_get_nonexistant);
TEST(sqsh_cat_fragment);
TEST(sqsh_cat_datablock_and_fragment);
TEST(sqsh_cat_size_overflow);
TEST(sqsh_test_uid_and_gid);
TEST(sqsh_test_extended_dir);
TEST(sqsh_test_xattr);
TEST_OFF(fuzz_crash_1); // Fails since the library sets up tables
TEST_OFF(fuzz_crash_2); // Fails since the library sets up tables
TEST_OFF(fuzz_crash_3); // Fails since the library sets up tables
TEST_OFF(fuzz_crash_4);
TEST_OFF(fuzz_crash_5);
TEST_OFF(fuzz_crash_6);
TEST_OFF(fuzz_crash_7);
DEFINE_END
