/******************************************************************************
 *                                                                            *
 * Copyright (c) 2021, Enno Boland <g@s01.de>                                 *
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
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : integration
 * @created     : Monday Oct 11, 2021 21:43:12 CEST
 */

#include "../gen/squash_image.h"
#include "../src/context/directory_context.h"
#include "../src/context/file_context.h"
#include "../src/context/inode_context.h"
#include "../src/context/superblock_context.h"
#include "../src/data/superblock.h"
#include "../src/error.h"
#include "../src/resolve_path.h"
#include "common.h"
#include "test.h"
#include <stdint.h>

static void
squash_ls() {
	int rv;
	char *name;
	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	struct SquashDirectoryContext dir = {0};
	struct SquashDirectoryIterator iter = {0};
	rv = squash_superblock_init(
			&superblock, squash_image, sizeof(squash_image));
	assert(rv == 0);

	rv = squash_inode_load(
			&inode, &superblock,
			squash_data_superblock_root_inode_ref(superblock.superblock));
	assert(rv == 0);

	rv = squash_directory_init(&dir, &superblock, &inode);
	assert(rv == 0);

	rv = squash_directory_iterator_init(&iter, &dir);
	assert(rv == 0);

	rv = squash_directory_iterator_next(&iter);
	assert(rv > 0);
	rv = squash_directory_iterator_name_dup(&iter, &name);
	assert(rv == 1);
	assert(strcmp("a", name) == 0);
	free(name);

	rv = squash_directory_iterator_next(&iter);
	assert(rv >= 0);
	rv = squash_directory_iterator_name_dup(&iter, &name);
	assert(rv == 1);
	assert(strcmp("b", name) == 0);
	free(name);

	rv = squash_directory_iterator_next(&iter);
	// End of file list
	assert(rv == 0);

	rv = squash_directory_iterator_cleanup(&iter);
	assert(rv == 0);

	rv = squash_directory_cleanup(&dir);
	assert(rv == 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
	assert(rv == 0);
}

static void
squash_cat_fragment() {
	int rv;
	const uint8_t *data;
	size_t size;
	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	struct SquashFileContext file = {0};
	rv = squash_superblock_init(
			&superblock, squash_image, sizeof(squash_image));
	assert(rv == 0);

	rv = squash_resolve_path(&inode, &superblock, "a");
	assert(rv == 0);

	rv = squash_file_init(&file, &inode);
	assert(rv == 0);

	size = squash_inode_file_size(&inode);
	assert(size == 2);

	rv = squash_file_read(&file, size);
	assert(rv == 0);

	data = squash_file_data(&file);
	assert(memcmp(data, "a\n", size) == 0);

	rv = squash_file_cleanup(&file);
	assert(rv == 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
	assert(rv == 0);
}

static void
squash_cat_datablock_and_fragment() {
	int rv;
	const uint8_t *data;
	size_t size;
	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	struct SquashFileContext file = {0};
	rv = squash_superblock_init(
			&superblock, squash_image, sizeof(squash_image));
	assert(rv == 0);

	rv = squash_resolve_path(&inode, &superblock, "b");
	assert(rv == 0);

	rv = squash_file_init(&file, &inode);
	assert(rv == 0);

	size = squash_inode_file_size(&inode);
	assert(size == 1050000);

	rv = squash_file_read(&file, size);
	assert(rv == 0);

	data = squash_file_data(&file);
	for (int i = 0; i < size; i++) {
		assert(data[i] == 'b');
	}

	rv = squash_file_cleanup(&file);
	assert(rv == 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
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

	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	rv = squash_superblock_init(&superblock, input, sizeof(input));
	assert(rv == 0);

	rv = squash_resolve_path(&inode, &superblock, "");
	assert(rv < 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
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

	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	rv = squash_superblock_init(&superblock, input, sizeof(input));
	assert(rv == 0);

	rv = squash_resolve_path(&inode, &superblock, "");
	assert(rv < 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
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

	struct SquashSuperblockContext superblock = {0};
	struct SquashInodeContext inode = {0};
	rv = squash_superblock_init(&superblock, input, sizeof(input));
	assert(rv == 0);

	rv = squash_resolve_path(&inode, &superblock, "");
	assert(rv < 0);

	rv = squash_inode_cleanup(&inode);
	assert(rv == 0);

	rv = squash_superblock_cleanup(&superblock);
	assert(rv == 0);
}

DEFINE
TEST(squash_ls);
TEST(squash_cat_fragment);
TEST(squash_cat_datablock_and_fragment);
TEST(fuzz_crash_1);
TEST(fuzz_crash_2);
TEST(fuzz_crash_3);
DEFINE_END