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
 * @file         lzma.c
 */

#include "../../include/sqsh_compression_private.h"

#include "../../include/sqsh_error.h"

#include <lzma.h>

int
sqsh_extract_lzma(
		uint8_t *target, size_t *target_size, const uint8_t *compressed,
		const size_t compressed_size) {
	lzma_ret rv = LZMA_OK;
	lzma_stream strm = LZMA_STREAM_INIT;

	rv = lzma_alone_decoder(&strm, UINT64_MAX);
	if (rv != LZMA_OK) {
		lzma_end(&strm);
		return -SQSH_ERROR_COMPRESSION_DECOMPRESS;
	}

	lzma_action action = LZMA_RUN;

	strm.next_in = compressed;
	strm.avail_in = compressed_size;

	strm.next_out = target;
	strm.avail_out = *target_size;

	action = LZMA_FINISH;

	if (lzma_code(&strm, action) != LZMA_OK) {
		rv = -SQSH_ERROR_COMPRESSION_DECOMPRESS;
	}

	*target_size = strm.avail_out;
	lzma_end(&strm);

	return rv;
}
