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
 * @file         table.c
 */

#include <sqsh.h>
#include <sqsh_context.h>
#include <sqsh_error.h>
#include <sqsh_table.h>
#include <stdint.h>
#include <string.h>

typedef const __attribute__((aligned(1))) uint64_t unaligned_uint64_t;
static uint64_t
lookup_table_get(const struct SqshTable *table, sqsh_index_t index) {
	unaligned_uint64_t *lookup_table =
			(unaligned_uint64_t *)sqsh_mapping_data(&table->lookup_table);

	return lookup_table[index];
}

int
sqsh_table_init(
		struct SqshTable *table, struct Sqsh *sqsh, sqsh_index_t start_block,
		size_t element_size, size_t element_count) {
	int rv = 0;
	size_t byte_size;
	size_t table_size;
	size_t lookup_table_size;
	size_t lookup_table_count;
	struct SqshMapper *mapper = sqsh_mapper(sqsh);

	if (MULT_OVERFLOW(element_size, element_count, &table_size)) {
		return -SQSH_ERROR_INTEGER_OVERFLOW;
	}

	lookup_table_count =
			SQSH_DEVIDE_CEIL(table_size, SQSH_METABLOCK_BLOCK_SIZE);

	if (MULT_OVERFLOW(
				lookup_table_count, sizeof(uint64_t), &lookup_table_size)) {
		return -SQSH_ERROR_INTEGER_OVERFLOW;
	}

	rv = sqsh_mapper_map(
			&table->lookup_table, mapper, start_block, lookup_table_size);
	if (rv < 0) {
		return rv;
	}

	table->sqsh = sqsh;
	table->element_size = element_size;
	table->element_count = element_count;
	if (MULT_OVERFLOW(element_size, element_count, &byte_size)) {
		rv = -SQSH_ERROR_INTEGER_OVERFLOW;
		goto out;
	}

out:
	return rv;
}

int
sqsh_table_get(
		const struct SqshTable *table, sqsh_index_t index, void *target) {
	int rv = 0;
	struct Sqsh *sqsh = table->sqsh;
	struct SqshMetablockContext metablock = {0};
	struct SqshBuffer buffer = {0};
	uint64_t lookup_index =
			index * table->element_size / SQSH_METABLOCK_BLOCK_SIZE;
	uint64_t metablock_address = lookup_table_get(table, lookup_index);
	uint64_t element_index =
			(index * table->element_size) % SQSH_METABLOCK_BLOCK_SIZE;

	if (rv < 0) {
		goto out;
	}

	rv = sqsh_metablock_init(&metablock, sqsh, metablock_address);
	if (rv < 0) {
		goto out;
	}

	rv = sqsh_metablock_to_buffer(&metablock, &buffer);
	if (rv < 0) {
		goto out;
	}

	memcpy(target, &sqsh_buffer_data(&buffer)[element_index],
		   table->element_size);

out:
	sqsh_metablock_cleanup(&metablock);
	sqsh_buffer_cleanup(&buffer);
	return rv;
}

int
sqsh_table_cleanup(struct SqshTable *table) {
	sqsh_mapping_unmap(&table->lookup_table);
	return 0;
}