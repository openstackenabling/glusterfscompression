/**
*
#    Copyright (C) 2013 Intel Corporation.  All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
 * target: compress & decompress data for storage
 * use ISA-L library to tuning the compress
 * zlib for decompress
 */
#ifndef __COMPRESS_H_
#define __COMPRESS_H_

#include "com-common.h"

typedef struct {
	Inner_fd_t *_fd;
	fd_t *_cached_fd;
	LZ_Stream1 *_cstream;
} compress_fd_t;

typedef struct Item {
	compress_fd_t *_value;
	struct Item *_next;
} compress_pool_node_t;

compress_pool_node_t * com_files_pool;

inline void init_com_file_pool();
inline void rm_fd_from_pool(fd_t *fd);
inline void set_fd_to_pool(xlator_t *xlator, fd_t *fd, int cfd, char *real_path);
inline compress_fd_t* get_fd_from_pool(fd_t *fd);
void buf_flush(compress_fd_t *cfd, int end);
int add_to_buffer(compress_fd_t *cfd, UINT8 *data, UINT32 len);

#endif
