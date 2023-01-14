/******************************************************************************
 *                                                                            *
 * Copyright (c) 2023, Enno Boland <g@s01.de>                                 *
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
 * @file         sqsh_context.h
 */

#ifndef SQSH_CONTEXT_PRIVATE_H
#define SQSH_CONTEXT_PRIVATE_H

#include "sqsh_mapper.h"
#include "sqsh_primitive.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Sqsh;

// context/compression_options_context.c

/**
 * @brief The compression options context is used to store the
 * compression options for a specific compression algorithm.
 */
struct SqshCompressionOptionsContext {
	uint16_t compression_id;
	struct SqshBuffer buffer;
};

/**
 * @brief Initialize the compression options context.
 * @memberof SqshCompressionOptionsContext
 * @param context the compression options context
 * @param sqsh the Sqsh struct
 */
SQSH_NO_UNUSED int sqsh__compression_options_init(
		struct SqshCompressionOptionsContext *context, struct Sqsh *sqsh);

/**
 * @brief Frees the resources used by the compression options context.
 * @memberof SqshCompressionOptionsContext
 * @param context the compression options context
 */
int sqsh__compression_options_cleanup(
		struct SqshCompressionOptionsContext *context);

// context/file_context.c

/**
 * @brief The SqshFileContext struct
 *
 * This struct is used to assemble file contents.
 */
struct SqshFileContext {
	/**
	 * @privatesection
	 */
	struct SqshMapper *mapper;
	struct SqshFragmentTable *fragment_table;
	struct SqshInodeContext *inode;
	struct SqshBuffer buffer;
	struct SqshCompression *compression;
	uint64_t seek_pos;
	uint32_t block_size;
};

/**
 * @internal
 * @brief Initializes a SqshFileContext struct.
 * @memberof SqshFileContext
 * @param context The file context to initialize.
 * @param inode The inode context to retrieve the file contents from.
 * @return 0 on success, less than 0 on error.
 */
SQSH_NO_UNUSED int sqsh__file_init(
		struct SqshFileContext *context, struct SqshInodeContext *inode);

/**
 * @internal
 * @brief Frees the resources used by the file context.
 * @memberof SqshFileContext
 * @param context The file context to clean up.
 */
int sqsh__file_cleanup(struct SqshFileContext *context);

// context/metablock_context.c

#define SQSH_METABLOCK_BLOCK_SIZE 8192

/**
 * @brief The SqshMetablockContext struct
 *
 * The SqshMetablockContext struct contains all information about a
 * metablock.
 */
struct SqshMetablockContext {
	struct SqshMapping mapping;
	struct SqshBuffer buffer;
	struct SqshCompression *compression;
};

/**
 * @brief sqsh__metablock_context_init
 * @param context The SqshMetablockContext to initialize.
 * @param sqsh The Sqsh struct.
 * @param address The address of the metablock.
 * @return 0 on success, less than 0 on error.
 */
int sqsh__metablock_init(
		struct SqshMetablockContext *context, struct Sqsh *sqsh,
		uint64_t address);

uint32_t
sqsh__metablock_compressed_size(const struct SqshMetablockContext *context);

SQSH_NO_UNUSED int sqsh__metablock_to_buffer(
		struct SqshMetablockContext *context, struct SqshBuffer *buffer);

int sqsh__metablock_cleanup(struct SqshMetablockContext *context);

// context/metablock_stream_context.c

struct SqshMetablockStreamContext {
	struct Sqsh *sqsh;
	struct SqshBuffer buffer;
	uint64_t base_address;
	uint64_t current_address;
	uint16_t buffer_offset;
};

SQSH_NO_UNUSED int sqsh__metablock_stream_init(
		struct SqshMetablockStreamContext *context, struct Sqsh *sqsh,
		uint64_t address, uint64_t max_address);

SQSH_NO_UNUSED int sqsh__metablock_stream_seek_ref(
		struct SqshMetablockStreamContext *context, uint64_t ref);

SQSH_NO_UNUSED int sqsh__metablock_stream_seek(
		struct SqshMetablockStreamContext *context, uint64_t address_offset,
		uint32_t buffer_offset);

SQSH_NO_UNUSED int sqsh__metablock_stream_more(
		struct SqshMetablockStreamContext *context, uint64_t size);

const uint8_t *
sqsh__metablock_stream_data(const struct SqshMetablockStreamContext *context);

size_t
sqsh__metablock_stream_size(const struct SqshMetablockStreamContext *context);

int sqsh__metablock_stream_cleanup(struct SqshMetablockStreamContext *context);

// context/inode_context.c

struct SqshInodeContext {
	struct SqshMetablockStreamContext metablock;
	struct Sqsh *sqsh;
};

/**
 * @internal
 * @brief Initialize the inode context from a inode reference. inode references
 * @memberof SqshInodeContext
 * are descriptors of the physical location of an inode inside the inode table.
 * They are diffrent from the inode number. In doubt use the inode number.
 *
 * @param context The inode context to initialize.
 * @param sqsh The sqsh context.
 * @param inode_ref The inode reference.
 * @return int 0 on success, less than 0 on error.
 */
SQSH_NO_UNUSED int sqsh__inode_init(
		struct SqshInodeContext *context, struct Sqsh *sqsh,
		uint64_t inode_ref);
/**
 * @internal
 * @brief cleans up the inode context.
 * @memberof SqshInodeContext
 * @param context The inode context.
 * @return int 0 on success, less than 0 on error.
 */
int sqsh__inode_cleanup(struct SqshInodeContext *context);

#ifdef __cplusplus
}
#endif
#endif /* end of include guard SQSH_CONTEXT_PRIVATE_H */