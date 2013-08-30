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

#ifndef __COM_COMMON_H__
#define __COM_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
//isa-l gzip library
#include "igzip_lib.h"
//zlib library
#include "zlib.h"
//glusterfs headers
#include "glusterfs.h"
#include "xlator.h"
#include "logging.h"

#define BUF_CAPACITY 655360

typedef struct {
	int *array;
	int cnt;
} com_isa_cbk_t;

typedef struct {
	UINT8 *_buf;
	UINT32 _capacity;
	UINT32 _len;
} source_pool_t;

source_pool_t *inbuf;
source_pool_t *outbuf;

typedef struct {
	int _fd;
	int _offset;
	char *_real_path;
}Inner_fd_t;

inline void init_inner_buffer();
inline void get_fd_from_ctx(xlator_t *xlator, fd_t *fd, void **value);
#endif
