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
#include "com-common.h"

inline void init_inner_buffer() {
	inbuf = (source_pool_t *) calloc(1, sizeof(source_pool_t));
	inbuf->_buf = (UINT8 *) calloc(BUF_CAPACITY, sizeof(UINT8));
	inbuf->_capacity = BUF_CAPACITY;
	outbuf = (source_pool_t *) calloc(1, sizeof(source_pool_t));
	outbuf->_buf = (UINT8 *) calloc(BUF_CAPACITY, sizeof(UINT8));
	outbuf->_capacity = BUF_CAPACITY;

}

inline void get_fd_from_ctx(xlator_t *xlator, fd_t *fd, void **value) {
	uint64_t tmp = 0;
	fd_ctx_get(fd, xlator, &tmp);
	value = (void *)(long)tmp;
}
