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

#include "decompress.h"

static char *decom_post = ".decom";

//init decompress files pool
inline void init_decom_file_pool() {
	decom_files_pool = (decompress_pool_node_t *) malloc(sizeof(decompress_pool_node_t));
	decom_files_pool->_next = NULL;
	decom_files_pool->_value = NULL;
}

/**
 * Find the decompress fd from cached files pool
 */
inline decompress_fd_t* get_fd_from_decoms(char *real_path) {
	decompress_pool_node_t *ptr = decom_files_pool->_next;
	while( ptr != NULL ) {
		if(ptr->_value && strcmp(ptr->_value->_sfd->_real_path, real_path) == 0) {
			return ptr->_value;
		}
	}
	return NULL;
}

/*
 * Find fd by fd_t
 */
inline decompress_fd_t* get_fd_from_decoms_0(fd_t *fd) {
	decompress_pool_node_t *ptr = decom_files_pool->_next;
	while( ptr != NULL ) {
		if(ptr->_value && ptr->_value->_cached_fd == fd) {
			return ptr->_value;
		}
	}
	return NULL;
}


//todo FIXME
void *decom_thread(void *arg) {
	if (arg) {
		decompress_fd_t *ptr = (decompress_fd_t *) arg;
		pthread_mutex_lock(ptr->_lock);
		//decompress files
		char d_in[BUF_SIZE], d_out[BUF_SIZE];
		int ret = 0;
		z_stream *d_stream = ptr->_dstream;
		d_stream->zfree = Z_NULL;
		d_stream->zalloc = Z_NULL;
		d_stream->opaque = Z_NULL;
		d_stream->avail_in = 0;
		d_stream->next_in = d_in;
		ret = inflateInit2(d_stream, MAX_WBITS+32);
		if (ret != Z_OK)
			printf("Init defalt result is %d\n", ret);
		int sfd = ptr->_sfd->_fd;
		int s_off = 0;
		int tfd = ptr->_rfd->_fd;
		int t_off =	0;
		do {
			d_stream->next_in = d_in;
			d_stream->avail_in = pread(sfd, d_in, 8192, s_off);
			//d_stream.avail_in = fread(d_in, 1, BUF_SIZE, IN);
			//printf("Read Avail in is %d\n", d_stream->avail_in);
			do {
				d_stream->next_out = d_out;
				d_stream->avail_out = BUF_SIZE;
				ret = inflate(d_stream, Z_NO_FLUSH);
				//printf("decompress result is %d\n", ret);
				t_off += pwrite(tfd, d_out, BUF_SIZE - d_stream->avail_out, t_off);
				//t_off += fwrite(d_out, 1, BUF_SIZE- d_stream.avail_out, OUT);
			} while(d_stream->avail_out == 0);
			s_off += BUF_SIZE;
		} while(ret != Z_STREAM_END);

		ret = inflateEnd(d_stream);
		printf("At last ret is %d\n", ret);
		pthread_mutex_unlock(ptr->_lock);

	}
	return NULL;
}

int open_decompress_file (char *real_path, fd_t *fd, int sfd) {
	char *decom_file_path = (char *) malloc (6+strlen(real_path)+1);
	strncpy(decom_file_path, real_path, strlen(real_path));
	strncpy(decom_file_path + strlen(real_path), decom_post, 6);
	decom_file_path[strlen(real_path) + 6] = '\0';
	printf("Open the decompress file for %s target file is: %s\n", real_path, decom_file_path);

	//todo real path has exist, just open for read
	// only can write once & read many times
	int has_exist = access(decom_file_path, F_OK); //0 exist, others not
	if (!has_exist) {
		printf("File %s has exist no need to decompress again\n ", decom_file_path);
	}
	int dfd = open(decom_file_path, O_CREAT|O_RDWR, 0);
	if (dfd == -1) {
		printf("Open file error %d\n", errno);
		return -1;
	} else {
		printf("Open successful for decompressing files\n");
		decompress_fd_t *tmp = (decompress_fd_t *) malloc(sizeof(decompress_fd_t));
		tmp->_cached_fd = fd;
		tmp->_dstream = (z_stream *) malloc(sizeof (z_stream));

		tmp->_sfd = (Inner_fd_t *) malloc(sizeof(Inner_fd_t));
		tmp->_sfd->_fd = sfd;
		tmp->_sfd->_real_path = real_path;
		tmp->_sfd->_offset = 0;

		tmp->_rfd = (Inner_fd_t *) malloc(sizeof(Inner_fd_t));
		tmp->_rfd->_fd = dfd;
		tmp->_rfd->_real_path = decom_file_path;
		tmp->_rfd->_offset = 0;

		tmp->_thread = (pthread_t *) malloc(sizeof(pthread_t));
		tmp->_lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(tmp->_lock, NULL);
		//add to decompress files pool
		decompress_pool_node_t *node = (decompress_pool_node_t *)malloc (sizeof(decompress_pool_node_t));
		node->_value = tmp;
		node->_next = decom_files_pool->_next;
		decom_files_pool->_next = node;

		//if file not exist run a new thread to decompress files
		if (has_exist) {
			pthread_create(tmp->_thread, NULL, decom_thread, (void *)tmp);
		}
		return dfd;
	}
}
