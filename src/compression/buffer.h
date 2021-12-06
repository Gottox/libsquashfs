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
 * @file        : compression
 * @created     : Sunday Sep 05, 2021 10:50:12 CEST
 */

#include "../utils.h"

#include "../data/superblock.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef EXTRACTOR_H

#define EXTRACTOR_H

struct HsqsSuperblockContext;

struct HsqsBuffer {
	const struct HsqsCompressionImplementation *impl;
	int block_size;
	uint8_t *data;
	size_t size;
};

HSQS_NO_UNUSED int hsqs_buffer_new(
		struct HsqsBuffer **context, int compression_id, int block_size);

HSQS_NO_UNUSED int hsqs_buffer_init(
		struct HsqsBuffer *compression, int compression_id, int block_size);

HSQS_NO_UNUSED int hsqs_buffer_append(
		struct HsqsBuffer *compression, const uint8_t *source,
		const size_t source_size, bool is_compressed);

const uint8_t *hsqs_buffer_data(const struct HsqsBuffer *buffer);
size_t hsqs_buffer_size(const struct HsqsBuffer *buffer);

int hsqs_buffer_cleanup(struct HsqsBuffer *compression);

int hsqs_buffer_free(struct HsqsBuffer *compression);

#endif /* end of include guard EXTRACTOR_H */
