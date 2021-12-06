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
 * @file        : superblock_context
 * @created     : Monday Oct 11, 2021 13:41:47 CEST
 */

#include "superblock_context.h"
#include "../data/superblock.h"
#include "../error.h"
#include <stdint.h>

const static uint32_t SUPERBLOCK_MAGIC = 0x73717368;
const static uint64_t NO_SEGMENT = 0xFFFFFFFFFFFFFFFF;

int
hsqs_superblock_init(
		struct HsqsSuperblockContext *context,
		struct HsqsMemoryMapper *mapper) {
	int rv = 0;
	rv = hsqs_mapper_map(&context->map, mapper, 0, HSQS_SIZEOF_SUPERBLOCK);
	if (rv < 0) {
		return -HSQS_ERROR_SUPERBLOCK_TOO_SMALL;
	}
	const struct HsqsSuperblock *superblock =
			(const struct HsqsSuperblock *)hsqs_map_data(&context->map);

	if (hsqs_data_superblock_magic(superblock) != SUPERBLOCK_MAGIC) {
		return -HSQS_ERROR_WRONG_MAGIG;
	}

	if (hsqs_data_superblock_block_log(superblock) !=
		hsqs_log2_u32(hsqs_data_superblock_block_size(superblock))) {
		return -HSQS_ERROR_BLOCKSIZE_MISSMATCH;
	}

	if (hsqs_data_superblock_bytes_used(superblock) >
		hsqs_mapper_size(mapper)) {
		return -HSQS_ERROR_SIZE_MISSMATCH;
	}

	context->superblock = superblock;
	context->mapper = mapper;

	if (hsqs_data_superblock_flags(context->superblock) &
		HSQS_SUPERBLOCK_COMPRESSOR_OPTIONS) {
		rv = hsqs_compression_options_init(
				&context->compression_options, context);
		if (rv < 0) {
			hsqs_superblock_cleanup(context);
			goto out;
		}
	}

	uint64_t id_table_start =
			hsqs_data_superblock_id_table_start(context->superblock);
	uint64_t xattr_table_start =
			hsqs_data_superblock_xattr_id_table_start(context->superblock);
	uint64_t export_table_start =
			hsqs_data_superblock_export_table_start(context->superblock);

	rv = hsqs_table_init(
			&context->id_table, context, id_table_start, sizeof(uint32_t),
			hsqs_data_superblock_id_count(context->superblock));

	if (rv < 0) {
		goto out;
	}

	if (xattr_table_start != NO_SEGMENT) {
		rv = hsqs_xattr_table_init(&context->xattr_table, context);
		if (rv < 0) {
			hsqs_superblock_cleanup(context);
			goto out;
		}
	}
	if (export_table_start != NO_SEGMENT) {
		rv = hsqs_table_init(
				&context->export_table, context, export_table_start,
				sizeof(uint64_t),
				hsqs_data_superblock_inode_count(context->superblock));
		if (rv < 0) {
			hsqs_superblock_cleanup(context);
			goto out;
		}
	}
	rv = hsqs_fragment_table_init(&context->fragment_table, context);
	if (rv < 0) {
		hsqs_superblock_cleanup(context);
		goto out;
	}
out:
	return rv;
}

const void *
hsqs_superblock_data_from_offset(
		const struct HsqsSuperblockContext *context, uint64_t offset) {
	const uint8_t *tmp = (uint8_t *)context->superblock;
	if (offset > hsqs_superblock_bytes_used(context)) {
		return NULL;
	}
	if (offset < HSQS_SIZEOF_SUPERBLOCK) {
		return NULL;
	}

	return &tmp[offset];
}

enum HsqsSuperblockCompressionId
hsqs_superblock_compression_id(const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_compression_id(context->superblock);
}

uint64_t
hsqs_superblock_directory_table_start(
		const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_directory_table_start(context->superblock);
}

uint64_t
hsqs_superblock_fragment_table_start(
		const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_fragment_table_start(context->superblock);
}

uint64_t
hsqs_superblock_inode_table_start(const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_inode_table_start(context->superblock);
}

uint64_t
hsqs_superblock_inode_root_ref(const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_root_inode_ref(context->superblock);
}

bool
hsqs_superblock_has_fragments(const struct HsqsSuperblockContext *context) {
	return !(
			hsqs_data_superblock_flags(context->superblock) &
			HSQS_SUPERBLOCK_NO_FRAGMENTS);
}

uint32_t
hsqs_superblock_block_size(const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_block_size(context->superblock);
}

uint32_t
hsqs_superblock_fragment_entry_count(
		const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_fragment_entry_count(context->superblock);
}

uint64_t
hsqs_superblock_bytes_used(const struct HsqsSuperblockContext *context) {
	return hsqs_data_superblock_bytes_used(context->superblock);
}

struct HsqsTableContext *
hsqs_superblock_id_table(struct HsqsSuperblockContext *context) {
	return &context->id_table;
}

struct HsqsTableContext *
hsqs_superblock_export_table(struct HsqsSuperblockContext *context) {
	return &context->export_table;
}

struct HsqsXattrTableContext *
hsqs_superblock_xattr_table(struct HsqsSuperblockContext *context) {
	return &context->xattr_table;
}

const struct HsqsCompressionOptionsContext *
hsqs_superblock_compression_options(
		const struct HsqsSuperblockContext *context) {
	if (hsqs_data_superblock_flags(context->superblock) &
		HSQS_SUPERBLOCK_COMPRESSOR_OPTIONS) {
		return &context->compression_options;
	} else {
		return NULL;
	}
}

int
hsqs_superblock_cleanup(struct HsqsSuperblockContext *superblock) {
	hsqs_table_cleanup(&superblock->id_table);
	hsqs_xattr_table_cleanup(&superblock->xattr_table);
	hsqs_table_cleanup(&superblock->export_table);
	hsqs_fragment_table_cleanup(&superblock->fragment_table);
	hsqs_compression_options_cleanup(&superblock->compression_options);
	hsqs_map_unmap(&superblock->map);
	return 0;
}
