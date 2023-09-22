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
 * @file         xattr_iterator.c
 */

#include <sqsh_reader_private.h>

#include <sqsh_common_private.h>
#include <sqsh_error.h>

#include <assert.h>

static bool
reader_iterator_next(struct SqshReader *reader, size_t desired_size, int *err) {
	bool has_next =
			reader->iterator_impl->next(reader->iterator, desired_size, err);
	if (has_next == false && *err == 0) {
		*err = -SQSH_ERROR_OUT_OF_BOUNDS;
		return false;
	} else {
		return has_next;
	}
}

int
sqsh__reader_init(
		struct SqshReader *reader,
		const struct SqshReaderIteratorImpl *iterator_impl, void *iterator) {
	reader->data = NULL;
	reader->size = 0;
	reader->offset = 0;
	reader->iterator_offset = 0;
	reader->iterator_impl = iterator_impl;
	reader->iterator = iterator;
	return cx_buffer_init(&reader->buffer);
}

/**
 * @brief copies data from the iterator to the buffer until the buffer
 * reaches the desired size.
 *
 * @param      reader  The reader
 * @param      offset  The offset
 * @param      size    The size
 *
 * @return     0 on success, negative on error.
 */
static int
reader_fill_buffer(struct SqshReader *reader, size_t size) {
	int rv = 0;
	void *iterator = reader->iterator;
	const struct SqshReaderIteratorImpl *impl = reader->iterator_impl;
	struct CxBuffer *buffer = &reader->buffer;
	sqsh_index_t offset = reader->offset;
	sqsh_index_t iterator_offset = reader->iterator_offset;

	size_t remaining_size = size - cx_buffer_size(buffer);
	for (;;) {
		const uint8_t *data = impl->data(iterator);
		const size_t data_size = impl->size(iterator);
		const size_t copy_size = SQSH_MIN(data_size - offset, remaining_size);
		rv = cx_buffer_append(buffer, &data[offset], copy_size);
		if (rv < 0) {
			goto out;
		}

		offset = 0;
		remaining_size -= copy_size;
		if (remaining_size == 0) {
			break;
		}

		iterator_offset = size - remaining_size;
		reader_iterator_next(reader, remaining_size, &rv);
		if (rv < 0) {
			goto out;
		}
	}

	assert(size == cx_buffer_size(buffer));
	assert(iterator_offset != 0);

	reader->iterator_offset = iterator_offset;
	reader->offset = 0;
	reader->data = cx_buffer_data(buffer);
	reader->size = size;
out:
	return rv;
}

static int
handle_buffered(struct SqshReader *reader, sqsh_index_t offset, size_t size) {
	int rv = 0;
	struct CxBuffer new_buffer = {0};
	sqsh_index_t iterator_offset = reader->iterator_offset;

	struct CxBuffer *buffer = &reader->buffer;
	const uint8_t *buffer_data = cx_buffer_data(buffer);
	size_t buffer_size = cx_buffer_size(buffer);
	const size_t copy_size = buffer_size - offset;

	if (offset != 0) {
		rv = cx_buffer_init(&new_buffer);
		if (rv < 0) {
			goto out;
		}
		rv = cx_buffer_append(&new_buffer, &buffer_data[offset], copy_size);
		if (rv < 0) {
			goto out;
		}
		rv = cx_buffer_move(buffer, &new_buffer);
		if (rv < 0) {
			goto out;
		}
	}

	reader->offset = buffer_size - iterator_offset;
	rv = reader_fill_buffer(reader, size);

out:
	return rv;
}

static int
handle_mapped(struct SqshReader *reader, sqsh_index_t offset, size_t size) {
	int rv = 0;
	void *iterator = reader->iterator;
	const struct SqshReaderIteratorImpl *impl = reader->iterator_impl;

	if (SQSH_ADD_OVERFLOW(offset, reader->offset, &offset)) {
		rv = -SQSH_ERROR_INTEGER_OVERFLOW;
		goto out;
	}

	rv = impl->skip(iterator, &offset, size);
	if (rv < 0) {
		goto out;
	}

	reader->offset = offset;
	sqsh_index_t end_offset;
	if (SQSH_ADD_OVERFLOW(offset, size, &end_offset)) {
		rv = -SQSH_ERROR_INTEGER_OVERFLOW;
		goto out;
	}
	if (end_offset <= impl->size(iterator)) {
		const uint8_t *data = impl->data(iterator);
		reader->data = &data[offset];
		reader->size = size;
	} else {
		struct CxBuffer *buffer = &reader->buffer;
		cx_buffer_drain(buffer);
		rv = reader_fill_buffer(reader, size);
	}

out:
	return rv;
}

int
sqsh__reader_advance(
		struct SqshReader *reader, sqsh_index_t offset, size_t size) {
	if (offset >= reader->iterator_offset) {
		offset -= reader->iterator_offset;
		reader->iterator_offset = 0;
		return handle_mapped(reader, offset, size);
	} else {
		return handle_buffered(reader, offset, size);
	}
}

const uint8_t *
sqsh__reader_data(const struct SqshReader *reader) {
	return reader->data;
}

size_t
sqsh__reader_size(const struct SqshReader *reader) {
	return reader->size;
}

int
sqsh__reader_cleanup(struct SqshReader *reader) {
	return cx_buffer_cleanup(&reader->buffer);
}
