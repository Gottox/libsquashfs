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
 * @file         sqsh_easy.h
 */

#ifndef SQSH_CHROME_H
#define SQSH_CHROME_H

#include "sqsh_common.h"

#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SqshArchive;
struct SqshFile;

/***************************************
 * chrome/file.c
 */

/**
 * @brief checks if a file exists.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 *
 * @return true if the file exists, false otherwise.
 */
bool sqsh_easy_file_exists(struct SqshArchive *archive, const char *path);

/**
 * @brief retrieves the content of a file.
 *
 * The content is not null terminated. The size of the content can be retrieved
 * with sqsh_file_size. The returned pointer needs to be released with `free()`.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 *
 * @return The content of the file on success, NULL on error.
 */
uint8_t *sqsh_easy_file_content(struct SqshArchive *archive, const char *path);

/**
 * @brief retrieves the size of a file.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 *
 * @return The size of the file on success, 0 on error.
 */
size_t sqsh_easy_file_size(struct SqshArchive *archive, const char *path);

/**
 * @brief retrieves unix permissions of a file.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 *
 * @return The unix permissions of the file on success, 0 on error.
 */
mode_t sqsh_easy_file_permission(struct SqshArchive *archive, const char *path);

/**
 * @brief retrieves the modification time of a file.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 *
 * @return The modification time of the file on success, 0 on error.
 */
time_t sqsh_easy_file_mtime(struct SqshArchive *archive, const char *path);

/***************************************
 * chrome/directory.c
 */

/**
 * @brief retrieves the contents of a directory.
 *
 * The returned list needs to be released with `free()`.
 *
 * @param[in] archive  The sqsh archive context.
 * @param[in] path     The path the file or directory.
 * @param[out] err     Pointer to an int where the error code will be stored.
 *
 * @return A list of files and directories on success, NULL on error.
 */
char **sqsh_easy_directory_list(
		struct SqshArchive *archive, const char *path, int *err);

#ifdef __cplusplus
}
#endif
#endif /* SQSH_CHROME_H */
