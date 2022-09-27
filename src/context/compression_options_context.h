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
 * @file         compression_options_context.h
 */

#include "../primitive/buffer.h"
#include "../utils.h"

#ifndef COMPRESSION_OPTIONS_CONTEXT_H

#define COMPRESSION_OPTIONS_CONTEXT_H

struct Sqsh;
union SqshCompressionOptions;

struct SqshCompressionOptionsContext {
	uint16_t compression_id;
	struct SqshBuffer buffer;
};
enum SqshGzipStrategies {
	SQSH_GZIP_STRATEGY_NONE = 0x0,
	SQSH_GZIP_STRATEGY_DEFAULT = 0x0001,
	SQSH_GZIP_STRATEGY_FILTERED = 0x0002,
	SQSH_GZIP_STRATEGY_HUFFMAN_ONLY = 0x0004,
	SQSH_GZIP_STRATEGY_RLE = 0x0008,
	SQSH_GZIP_STRATEGY_FIXED = 0x0010,
};
enum SqshXzFilters {
	SQSH_XZ_FILTER_NONE = 0x0,
	SQSH_XZ_FILTER_X86 = 0x0001,
	SQSH_XZ_FILTER_POWERPC = 0x0002,
	SQSH_XZ_FILTER_IA64 = 0x0004,
	SQSH_XZ_FILTER_ARM = 0x0008,
	SQSH_XZ_FILTER_ARMTHUMB = 0x0010,
	SQSH_XZ_FILTER_SPARC = 0x0020,
};
enum SqshLz4Flags {
	SQS_LZ4_FLAG_NONE = 0x0,
	SQSH_LZ4_HIGH_COMPRESSION = 0x0001,
};
enum SqshLzoAlgorithm {
	SQSH_LZO_ALGORITHM_LZO1X_1 = 0x0000,
	SQSH_LZO_ALGORITHM_LZO1X_1_11 = 0x0001,
	SQSH_LZO_ALGORITHM_LZO1X_1_12 = 0x0002,
	SQSH_LZO_ALGORITHM_LZO1X_1_15 = 0x0003,
	SQSH_LZO_ALGORITHM_LZO1X_999 = 0x0004,
};

HSQS_NO_UNUSED int sqsh_compression_options_init(
		struct SqshCompressionOptionsContext *context, struct Sqsh *sqsh);

const union SqshCompressionOptions *sqsh_compression_options_data(
		const struct SqshCompressionOptionsContext *context);
size_t sqsh_compression_options_size(
		const struct SqshCompressionOptionsContext *context);

uint32_t sqsh_compression_options_gzip_compression_level(
		const struct SqshCompressionOptionsContext *options);
uint16_t sqsh_compression_options_gzip_window_size(
		const struct SqshCompressionOptionsContext *options);
enum SqshGzipStrategies sqsh_compression_options_gzip_strategies(
		const struct SqshCompressionOptionsContext *options);

uint32_t sqsh_compression_options_xz_dictionary_size(
		const struct SqshCompressionOptionsContext *options);
enum SqshXzFilters sqsh_compression_options_xz_filters(
		const struct SqshCompressionOptionsContext *options);

uint32_t sqsh_compression_options_lz4_version(
		const struct SqshCompressionOptionsContext *options);
uint32_t sqsh_compression_options_lz4_flags(
		const struct SqshCompressionOptionsContext *options);

uint32_t sqsh_compression_options_zstd_compression_level(
		const struct SqshCompressionOptionsContext *options);

enum SqshLzoAlgorithm sqsh_compression_options_lzo_algorithm(
		const struct SqshCompressionOptionsContext *options);
uint32_t sqsh_compression_options_lzo_compression_level(
		const struct SqshCompressionOptionsContext *options);

int
sqsh_compression_options_cleanup(struct SqshCompressionOptionsContext *context);

#endif /* end of include guard COMPRESSION_OPTIONS_CONTEXT_H */