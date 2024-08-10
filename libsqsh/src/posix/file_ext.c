/******************************************************************************
 *                                                                            *
 * Copyright (c) 2023-2024, Enno Boland <g@s01.de>                            *
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
 * @file         posix.c
 */

#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <sqsh_archive.h>
#include <sqsh_common_private.h>
#include <sqsh_error.h>
#include <sqsh_file_private.h>
#include <sqsh_posix_private.h>

int
sqsh_file_to_stream(const struct SqshFile *file, FILE *stream) {
	int rv = 0;
	struct SqshFileIterator iterator = {0};

	rv = sqsh__file_iterator_init(&iterator, file);
	if (rv < 0) {
		goto out;
	}

	while (sqsh_file_iterator_next(&iterator, SIZE_MAX, &rv)) {
		const uint8_t *data = sqsh_file_iterator_data(&iterator);
		const size_t size = sqsh_file_iterator_size(&iterator);
		const size_t written = fwrite(data, sizeof(uint8_t), size, stream);
		if (written > 0 && written != size) {
			rv = -errno;
			goto out;
		}
	}

out:
	sqsh__file_iterator_cleanup(&iterator);
	return rv;
}

struct FileIteratorMtBlock {
	struct FileIteratorMt *mt;
	uint64_t block_offset;
};

struct FileIteratorMt {
	struct SqshFile file;
	sqsh_file_iterator_mt_cb cb;
	uint32_t chunk_size;
	void *data;
	atomic_int rv;
	atomic_size_t remaining_blocks;

	struct FileIteratorMtBlock *blocks;
};

struct FileToStreamMt {
	struct FileIteratorMt mt;
	sqsh_file_to_stream_mt_cb cb;
	void *data;
	FILE *stream;
};

static void
file_iterator_mt_cleanup(struct FileIteratorMt *mt, int rv) {
	mt->cb(&mt->file, NULL, 0, mt->data, rv);
	sqsh__file_cleanup(&mt->file);
	free(mt->blocks);
	free(mt);
}

static void
iterator_worker(void *data) {
	int rv = 0, rv2 = 0;
	struct SqshFileIterator iterator = {0};

	struct FileIteratorMtBlock *block = data;
	struct FileIteratorMt *mt = block->mt;

	rv = sqsh__file_iterator_init(&iterator, &mt->file);
	if (rv < 0) {
		goto out;
	}

	sqsh_index_t offset = block->block_offset;
	rv = sqsh_file_iterator_skip(&iterator, &offset, 1);
	if (rv < 0) {
		goto out;
	}
	assert(offset == 0);

	mt->cb(&mt->file, &iterator, block->block_offset, mt->data, rv);

out:
	sqsh__file_iterator_cleanup(&iterator);

	assert(rv2 == 0);

	const size_t remaining_blocks = atomic_fetch_sub(&mt->remaining_blocks, 1);
	if (rv < 0) {
		atomic_store(&mt->rv, rv);
	}

	if (remaining_blocks == 1) {
		file_iterator_mt_cleanup(mt, atomic_load(&mt->rv));
	}
}

static void
file_iterator_mt(
		struct FileIteratorMt *mt, const struct SqshFile *file,
		struct SqshThreadpool *threadpool, sqsh_file_iterator_mt_cb cb,
		void *data, int rv) {
	if (rv < 0) {
		goto out;
	}
	const uint64_t inode_ref = sqsh_file_inode_ref(file);
	const struct SqshSuperblock *superblock =
			sqsh_archive_superblock(file->archive);
	uint32_t chunk_size = sqsh_superblock_block_size(superblock);

	const size_t block_count =
			SQSH_DIVIDE_CEIL(sqsh_file_size(file), chunk_size);

	mt->cb = cb;
	mt->data = data;
	mt->chunk_size = chunk_size;
	atomic_init(&mt->remaining_blocks, block_count);
	atomic_init(&mt->rv, 0);

	if (block_count == 0) {
		goto out;
	}

	rv = sqsh__file_init(&mt->file, file->archive, inode_ref);
	if (rv < 0) {
		goto out;
	}

	mt->blocks = calloc(sizeof(struct FileIteratorMtBlock), block_count);
	if (mt->blocks == NULL) {
		rv = -SQSH_ERROR_MALLOC_FAILED;
		goto out;
	}

	size_t block_offset = 0;
	for (sqsh_index_t i = 0; i < block_count; i++) {
		mt->blocks[i].mt = mt;
		mt->blocks[i].block_offset = block_offset;
		rv = cx_threadpool_schedule(
				&threadpool->pool, iterator_worker, &mt->blocks[i]);
		if (rv < 0) {
			goto out;
		}
		block_offset += chunk_size;
	}

out:
	if (rv < 0 || block_count == 0) {
		file_iterator_mt_cleanup(mt, rv);
	}
}

void
sqsh_file_iterator_mt(
		const struct SqshFile *file, struct SqshThreadpool *threadpool,
		sqsh_file_iterator_mt_cb cb, void *data) {
	int rv = 0;

	struct FileIteratorMt *mt = calloc(sizeof(struct FileIteratorMt), 1);
	if (mt == NULL) {
		rv = -SQSH_ERROR_MALLOC_FAILED;
	}

	file_iterator_mt(mt, file, threadpool, cb, data, rv);
}

static void
stream_worker(
		const struct SqshFile *file, const struct SqshFileIterator *iterator,
		sqsh_index_t offset, void *data, int err) {
	int rv = 0, rv2 = 0;
	FILE *stream = NULL;
	struct FileToStreamMt *mt = data;
	if (iterator == NULL) {
		mt->cb(file, mt->stream, mt->data, err);
		return;
	}

	int fd = dup(fileno(mt->stream));
	stream = fdopen(fd, "r+");
	if (stream == NULL) {
		close(fd);
		rv = -errno;
		goto out;
	}

	rv = fseeko64(stream, (off_t)offset, SEEK_SET);
	const uint8_t *iterator_data = sqsh_file_iterator_data(iterator);
	const size_t iterator_size = sqsh_file_iterator_size(iterator);
	const size_t written =
			fwrite(iterator_data, sizeof(uint8_t), iterator_size, stream);
	if (written != iterator_size) {
		rv = -errno;
		goto out;
	}

out:
	if (stream != NULL) {
		fclose(stream);
	}
	assert(rv2 == 0);

	if (rv < 0) {
		atomic_store(&mt->mt.rv, rv);
	}
}

void
sqsh_file_to_stream_mt(
		const struct SqshFile *file, struct SqshThreadpool *threadpool,
		FILE *stream, sqsh_file_to_stream_mt_cb cb, void *data) {
	int rv = 0;

	struct FileToStreamMt *mt = calloc(sizeof(struct FileToStreamMt), 1);
	if (mt == NULL) {
		rv = -SQSH_ERROR_MALLOC_FAILED;
	}
	mt->stream = stream;
	mt->cb = cb;
	mt->data = data;

	file_iterator_mt(&mt->mt, file, threadpool, stream_worker, mt, rv);
}
