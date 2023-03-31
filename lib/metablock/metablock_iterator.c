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
 * @file         metablock_iterator.c
 */

#include "../../include/sqsh_metablock_private.h"

#include "../../include/sqsh_archive.h"
#include "../../include/sqsh_data.h"
#include "../../include/sqsh_error.h"

#include <stdint.h>
#include <string.h>

#include "../../include/sqsh_archive_private.h"
#include "../../include/sqsh_compression_private.h"

int
sqsh__metablock_iterator_init(
		struct SqshMetablockIterator *iterator, struct SqshArchive *sqsh,
		uint64_t start_address, uint64_t upper_limit) {
	struct SqshMapManager *map_manager = sqsh_archive_map_manager(sqsh);

	iterator->size = 0;
	iterator->is_compressed = false;
	return sqsh__map_reader_init(
			&iterator->cursor, map_manager, start_address, upper_limit);
}

int
sqsh__metablock_iterator_next(struct SqshMetablockIterator *iterator) {
	int rv = 0;

	rv = sqsh__map_reader_advance(
			&iterator->cursor, iterator->size, SQSH_SIZEOF_METABLOCK);
	if (rv < 0) {
		goto out;
	}

	const struct SqshDataMetablock *metablock =
			(struct SqshDataMetablock *)sqsh__map_reader_data(
					&iterator->cursor);
	iterator->size = sqsh_data_metablock_size(metablock);
	iterator->is_compressed = sqsh_data_metablock_is_compressed(metablock);

	if (iterator->size > SQSH_METABLOCK_BLOCK_SIZE) {
		rv = -SQSH_ERROR_TODO;
		goto out;
	}

	rv = sqsh__map_reader_advance(
			&iterator->cursor, SQSH_SIZEOF_METABLOCK, iterator->size);

out:
	return rv;
}

int
sqsh__metablock_iterator_skip(
		struct SqshMetablockIterator *iterator, size_t amount) {
	int rv = 0;

	// TODO: replace _next calls with directly accessing the cursor and not
	// mapping the data, just the headers
	for (uint64_t i = 0; i < amount; i++) {
		rv = sqsh__metablock_iterator_next(iterator);
		if (rv < 0) {
			goto out;
		}
	}
out:
	return rv;
}

const uint8_t *
sqsh__metablock_iterator_data(const struct SqshMetablockIterator *iterator) {
	return sqsh__map_reader_data(&iterator->cursor);
}

uint64_t
sqsh__metablock_iterator_data_address(
		const struct SqshMetablockIterator *iterator) {
	// TODO: do not access the cursor directly, implement functions to get its
	// address
	return iterator->cursor.address + SQSH_SIZEOF_METABLOCK;
}

bool
sqsh__metablock_iterator_is_compressed(
		const struct SqshMetablockIterator *iterator) {
	return iterator->is_compressed;
}

size_t
sqsh__metablock_iterator_size(const struct SqshMetablockIterator *iterator) {
	return iterator->size;
}

int
sqsh__metablock_iterator_cleanup(struct SqshMetablockIterator *iterator) {
	return sqsh__map_reader_cleanup(&iterator->cursor);
}
