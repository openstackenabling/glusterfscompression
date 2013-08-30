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
#ifndef __DECOMPRESS_H_
#define __DECOMPRESS_H_

#include "com-common.h"

#define BUF_SIZE 8192

typedef struct {
	Inner_fd_t *_sfd; //source fd
	Inner_fd_t *_rfd; //real read fd
	fd_t *_cached_fd;
	z_stream *_dstream;
	pthread_mutex_t *_lock;
	pthread_t * _thread;
} decompress_fd_t;

typedef struct DecItem {
	decompress_fd_t *_value;
	struct DecItem *_next;
} decompress_pool_node_t;

decompress_pool_node_t * decom_files_pool;

inline void init_decom_file_pool();
inline decompress_fd_t* get_fd_from_decoms(char *real_path);
inline decompress_fd_t* get_fd_from_decoms_0(fd_t *fd);
void *decom_thread(void *arg);
int open_decompress_file (char *real_path, fd_t *fd, int sfd);

#endif
