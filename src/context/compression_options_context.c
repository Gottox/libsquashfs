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
 * @file        : compression_option_context
 * @created     : Tuesday Nov 30, 2021 15:30:08 CET
 */

#include "compression_options_context.h"
#include "../data/compression_options.h"
#include "../data/superblock.h"

int
hsqs_compression_options_init(
		struct HsqsCompressionOptionsContext *context,
		struct HsqsSuperblockContext *superblock) {
	int rv = 0;

	rv = hsqs_metablock_init(
			&context->metablock, superblock, HSQS_SIZEOF_SUPERBLOCK);
	if (rv < 0) {
		goto out;
	}

	// size of one decompresses the whole block
	rv = hsqs_metablock_more(&context->metablock, 1);
	if (rv < 0) {
		goto out;
	}

out:
	return rv;
}

const union HsqsCompressionOptions *
hsqs_compression_options(const struct HsqsCompressionOptionsContext *context) {
	return (const union HsqsCompressionOptions *)hsqs_metablock_data(
			&context->metablock);
}

size_t
hsqs_compression_options_size(
		const struct HsqsCompressionOptionsContext *context) {
	return hsqs_metablock_size(&context->metablock);
}

int
hsqs_compression_options_cleanup(
		struct HsqsCompressionOptionsContext *context) {
	hsqs_metablock_cleanup(&context->metablock);

	return 0;
}