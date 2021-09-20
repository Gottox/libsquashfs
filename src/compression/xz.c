/**
 * @author      : Enno Boland (mail@eboland.de)
 * @file        : xz
 * @created     : Sunday Sep 05, 2021 11:09:51 CEST
 */

#include <lzma.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <zconf.h>

#include "../data/compression_options.h"
#include "../error.h"
#include "compression.h"

static int
squash_xz_extract(const union SquashCompressionOptions *options,
		uint8_t *target, size_t *target_size, const uint8_t *compressed,
		const size_t compressed_size) {
	int rv = 0;
	size_t compressed_pos = 0;
	size_t target_pos = 0;
	uint64_t memlimit = UINT64_MAX;

	rv = lzma_stream_buffer_decode(&memlimit, 0, NULL, compressed,
			&compressed_pos, compressed_size, target, &target_pos,
			*target_size);

	*target_size = target_pos;

	if (rv != LZMA_OK) {
		return -SQUASH_ERROR_COMPRESSION_DECOMPRESS;
	}

	if (compressed_pos != compressed_size) {
		return -SQUASH_ERROR_COMPRESSION_DECOMPRESS;
	}
	return rv;
}

const struct SquashCompressionImplementation squash_compression_xz = {
		.extract = squash_xz_extract,
};
