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
#include "compress.h"

//init compress files pool
inline void init_com_file_pool() {
	com_files_pool = (compress_pool_node_t *) malloc(sizeof(compress_pool_node_t));
	com_files_pool->_next = NULL;
	com_files_pool->_value = NULL;
}

inline compress_fd_t* get_fd_from_pool(fd_t *fd) {
	compress_pool_node_t *ptr = com_files_pool->_next;
	while (ptr != NULL) {
		if (ptr->_value && ptr->_value->_cached_fd == fd)
			return ptr->_value;
	}
	return NULL;
}

inline void set_fd_to_pool(xlator_t *xlator, fd_t *fd, int cfd, char *real_path) {
	compress_fd_t *tmpfd = NULL;
	compress_pool_node_t *tmpnode;
	get_fd_from_ctx(xlator, fd, (void **)&tmpfd);
	if (tmpfd == NULL) {
		tmpfd = (compress_fd_t *) malloc(sizeof(compress_fd_t));
		if(tmpfd !=NULL) {
			tmpfd->_cached_fd = fd;
			Inner_fd_t *tfd = (Inner_fd_t *) malloc(sizeof(Inner_fd_t));
			tfd->_fd = cfd;
			tfd->_offset = 0;
			tfd->_real_path = real_path;
			tmpfd->_fd = tfd;
			tmpfd->_cstream = (LZ_Stream1 *) malloc(sizeof(LZ_Stream1));
			init_stream(tmpfd->_cstream);
			tmpnode = (compress_pool_node_t *) malloc(sizeof(compress_pool_node_t));
			if (tmpnode != NULL) {
				tmpnode->_value = tmpfd;
				tmpnode->_next = com_files_pool->_next;
				com_files_pool->_next = tmpnode;
			}
			fd_ctx_set(fd, xlator, (uint64_t)(long)tmpfd);
		}

	} else {
		printf("Has already set the fd value\n");
	}

}

inline void rm_fd_from_pool(fd_t *fd) {
	compress_pool_node_t *cur = com_files_pool->_next, *pre;
	pre = cur;
	while (cur != NULL) {
		if (cur->_value->_cached_fd == fd) {
			pre->_next = cur->_next;
			free(cur);
			cur = NULL;
			return;
		}
		pre = cur;
		cur = cur->_next;
	}
}

int add_to_buffer(compress_fd_t *cfd, UINT8 *data, UINT32 len) {
	UINT32 avail = inbuf->_capacity - inbuf->_len;
	int inneroff = 0;
	UINT32 remain = len;
	while (remain > 0) {
		if (remain > avail) {
			memcpy(inbuf->_buf + inbuf->_len, data + inneroff, avail);
			inneroff += avail;
			inbuf->_len += avail;
			buf_flush(cfd, 0);
			inbuf->_len = 0;
			remain -= avail;
			avail = inbuf->_capacity;
		} else {
			memcpy(inbuf->_buf + inbuf->_len, data + inneroff, remain);
			inbuf->_len += remain;
			remain = 0;
		}
	}
	return len;
}

//flush data to file
void buf_flush(compress_fd_t *cfd, int end) {
	LZ_Stream1 *cstream = cfd->_cstream;
	cstream->end_of_stream = end;
	cstream->next_in = inbuf->_buf;
	cstream->avail_in = inbuf->_len;
	//compress and write to file
	do {
		cstream->next_out = outbuf->_buf;
		cstream->avail_out = outbuf->_capacity;
		printf("Avail in %d, avail out is %d\n", cstream->avail_in, cstream->avail_out);
		fast_lz(cstream);
		outbuf->_len = outbuf->_capacity - cstream->avail_out;
		int retval = pwrite(cfd->_fd->_fd, outbuf->_buf, outbuf->_len, cfd->_fd->_offset);
		cfd->_fd->_offset += retval;
		printf("Have write %d, offset is %d\n", retval, cfd->_fd->_offset);
	} while (cstream->avail_out == 0);
	assert(cstream->avail_in == 0);
	//reset buff status
	outbuf->_len = 0;
	inbuf->_len = 0;
}
