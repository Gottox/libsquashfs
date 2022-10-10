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
 * @file         metablock_stream_context.h
 */

#include "../primitive/buffer.h"

#ifndef METABLOCK_STREAM_CONTEXT_H

#define METABLOCK_STREAM_CONTEXT_H

struct SqshMetablockStreamContext {
	struct Sqsh *sqsh;
	struct SqshBuffer buffer;
	uint64_t base_address;
	uint64_t current_address;
	uint16_t buffer_offset;
};

SQSH_NO_UNUSED int sqsh_metablock_stream_init(
		struct SqshMetablockStreamContext *context, struct Sqsh *sqsh,
		uint64_t address, uint64_t max_address);

SQSH_NO_UNUSED int sqsh_metablock_stream_seek_ref(
		struct SqshMetablockStreamContext *context, uint64_t ref);

SQSH_NO_UNUSED int sqsh_metablock_stream_seek(
		struct SqshMetablockStreamContext *context, uint64_t address_offset,
		uint32_t buffer_offset);

SQSH_NO_UNUSED int sqsh_metablock_stream_more(
		struct SqshMetablockStreamContext *context, uint64_t size);

const uint8_t *
sqsh_metablock_stream_data(const struct SqshMetablockStreamContext *context);

size_t
sqsh_metablock_stream_size(const struct SqshMetablockStreamContext *context);

int sqsh_metablock_stream_cleanup(struct SqshMetablockStreamContext *context);

#endif /* end of include guard METABLOCK_STREAM_CONTEXT_H */
