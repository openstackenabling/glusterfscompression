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
#include <ctype.h>
#include <sys/uio.h>
#include <assert.h>

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif

#include "com-isa.h"
#include "com-common.h"
#include "decompress.h"
#include "compress.h"
/**
 * A compress & decompress translator
 * use isa for data compress
 * use zlib for data decompress
 */

typedef struct {
	char *data;
	int len;
}create_cbk_private_t;

int32_t
com_create_cbk (call_frame_t *frame, void *cookie, xlator_t *this,
                   int32_t op_ret, int32_t op_errno, fd_t *fd,
                   inode_t *inode, struct iatt *buf, struct iatt *preparent,
                   struct iatt *postparent, dict_t *xdata){

	create_cbk_private_t *retcbk = (create_cbk_private_t *)cookie;

	if (retcbk != NULL) {
		int cfd = open(retcbk->data, O_CREAT|O_WRONLY, 0);
		if (cfd == -1) {
			printf("create file error for file %s\n", retcbk->data);
		}
		set_fd_to_pool(this, fd, cfd, retcbk->data);
	}

	STACK_UNWIND_STRICT(create, frame, op_ret, op_errno, fd, inode, buf, preparent, postparent, xdata);

	return 0;
}

int32_t com_create(call_frame_t *frame, xlator_t *this, loc_t *loc,
	int32_t flags, mode_t mode, mode_t umask, fd_t *fd, dict_t *xdata) {
	data_t *dir_data = NULL;
	dir_data = dict_get(this->next->options, "directory");
	char *real_path = (char *) malloc(strlen(dir_data->data) + strlen(loc->path) + 1);
	strcpy(real_path, dir_data->data);
	strcpy(real_path+strlen(dir_data->data), loc->path);
	printf("create file  path %s file name %s dir info is %s realpath is:%s\n", loc->path,
					loc->name, dir_data->data, real_path);

	create_cbk_private_t *retcbk = (create_cbk_private_t *)malloc(sizeof(create_cbk_private_t));
	retcbk->data = real_path;
	retcbk->len = strlen(real_path);
	STACK_WIND_COOKIE (frame, com_create_cbk, retcbk, FIRST_CHILD (this),
	                    FIRST_CHILD (this)->fops->create, loc, flags, mode,
	                    umask, fd, xdata);
	return 0;
}


//file exsit, compressed file, just open for read
//mark current open files
int32_t com_open(call_frame_t *frame, xlator_t *this, loc_t *loc, int32_t flags,
		fd_t *fd, dict_t *xdata) {
	data_t *dir_data = NULL;
	dir_data = dict_get(this->next->options, "directory");
	printf("open file  path %s file name %s\n", loc->path, loc->name);
	if (flags & O_WRONLY) {
	//only read allow
		printf("Open for write not allowed\n");
		STACK_UNWIND_STRICT(open, frame, -1, errno, NULL, NULL);
		return 0;
	} else {
		char *real_path = (char *) malloc(sizeof(char) * (strlen(loc->path) + strlen(dir_data->data) + 1));	
		strcpy(real_path, dir_data->data);
		strcpy(real_path + strlen(dir_data->data), loc->path);
	
		printf("Open file %s to read but you need to decompress it\n", real_path);
		//find the cached undecompress file, if finded open add pair for <fd_t, fd>
		// if not find, decompress file and cached the fd
		decompress_fd_t *dfd = get_fd_from_decoms(real_path);
		if (dfd == NULL) {
			int tfd = open(real_path, flags, 0);
			if (tfd == -1)
				goto out;
			int tdfd = open_decompress_file(real_path, fd, tfd);
			if (tdfd == -1)
				goto out;
			STACK_UNWIND_STRICT(open, frame, 0, errno, fd, NULL);
			return 0;
		} else {
			STACK_UNWIND_STRICT(open, frame,0, errno, dfd->_cached_fd, NULL);
			return 0;
		}
	}

out:
	{
		printf("Open file error\n");
		STACK_UNWIND_STRICT(open, frame, -1, errno, NULL, NULL);
		return 0;
	}

}

int32_t com_flush_cbk(call_frame_t *frame, void *cookie, xlator_t *this,
        int32_t op_ret, int32_t op_errno, dict_t *xdata) {
	STACK_UNWIND_STRICT(flush, frame, op_ret, op_errno, xdata);
	return 0;
}

//when flush code, to end the stream
int32_t com_flush(call_frame_t *frame, xlator_t *this, fd_t *fd, dict_t *xdata) {
	compress_fd_t *cfd = get_fd_from_pool(fd);
	if (cfd != NULL) {
		buf_flush(cfd, 1);
		STACK_WIND(frame, com_flush_cbk, FIRST_CHILD(this),
						FIRST_CHILD(this)->fops->flush, fd, xdata);
		return 0;
	}
	decompress_fd_t *dfd = get_fd_from_decoms_0(fd);
	if (dfd != NULL) {
		close(dfd->_rfd->_fd);
		close(dfd->_sfd->_fd);
		STACK_UNWIND_STRICT(flush, frame, 0, 0, NULL);
		return 0;
	}

}

//callback function for writev
int com_isa_writev_cbk(call_frame_t *frame, void *cookie, xlator_t *this,
		int32_t op_ret, int32_t op_errno, struct iatt *prebuf,
		struct iatt *postbuf, dict_t *xdata) {
	if (cookie == NULL) {
		STACK_UNWIND_STRICT(writev, frame, op_ret, op_errno, prebuf, postbuf,
				xdata);
		return 0;
	}
	com_isa_cbk_t *ret = (com_isa_cbk_t *) cookie;
	int i, writed = 0;
	for (i = 0; i < ret->cnt; i++) {
		writed += ret->array[i];
	}
	STACK_UNWIND_STRICT(writev, frame, writed, op_errno, prebuf, postbuf,
			xdata);
	return 0;
}

int
isa_com_fdstat (xlator_t *this, int fd, struct iatt *stbuf_p)
{
        int                    ret     = 0;
        struct stat            fstatbuf = {0, };
        struct iatt            stbuf = {0, };

        ret = fstat (fd, &fstatbuf);
        if (ret == -1)
                goto out;

        if (fstatbuf.st_nlink && !S_ISDIR (fstatbuf.st_mode))
                fstatbuf.st_nlink--;

        iatt_from_stat (&stbuf, &fstatbuf);
        /*
        ret = posix_fill_gfid_fd (this, fd, &stbuf);
        if (ret)
                gf_log_callingfn (this->name, GF_LOG_DEBUG, "failed to get gfid");

        posix_fill_ino_from_gfid (this, &stbuf);
		*/
        if (stbuf_p)
                *stbuf_p = stbuf;

out:
        return ret;
}

int32_t
isa_com_fstat_cbk (call_frame_t *frame, void *cookie, xlator_t *this, int32_t op_ret,
		int32_t op_errno, struct iatt *buf, dict_t *xdata) {
	STACK_UNWIND_STRICT (fstat, frame, op_ret, op_errno, buf, xdata);
}

int32_t
isa_com_fstat (call_frame_t *frame, xlator_t *this,
             fd_t *fd, dict_t *xdata){
	int32_t               op_ret   = -1;
	int32_t               op_errno = 0;
	struct iatt           buf      = {0,};

	decompress_fd_t *dfd = get_fd_from_decoms_0(fd);
    if (dfd == NULL) {
    	//not decompress fd, so deal by posix translator
    	STACK_WIND(frame, isa_com_fstat_cbk, FIRST_CHILD(this),FIRST_CHILD(this)->fops->fstat, fd, xdata);
    }

	op_ret = isa_com_fdstat(this, dfd->_rfd->_fd, &buf);
    if (op_ret == -1) {
    	op_errno =errno;
    	printf("fstat file error");
    	goto out;
    }
    op_ret = 0;
out:

	STACK_UNWIND_STRICT (fstat, frame, op_ret, op_errno, &buf, NULL);
    return 0;
}

int decom_isa_readv(call_frame_t *frame, xlator_t *this, fd_t *fd, size_t size,
		off_t offset, uint32_t flags, dict_t *xdata) {

	struct iatt            stbuf      = {0,};

	decompress_fd_t *dfd = get_fd_from_decoms_0(fd);
	if (dfd == NULL) {
		printf("Read Error for decompress fd is NULL\n");
	} else {
		int op_errno = 0, op_ret = 0;
		struct iobuf *iobuf = NULL;
		struct iovec *vec = NULL;
		struct iobref *iobref = NULL;

		iobuf = iobuf_get2(this->ctx->iobuf_pool, size);
		if (!iobuf) {
			op_errno = EINVAL;
			goto out;
		}

		printf("Read from %s the offset is %ld size is %ld\n", dfd->_rfd->_real_path, offset, size);
		pthread_mutex_lock(dfd->_lock);
		op_ret = pread(dfd->_rfd->_fd, iobuf->ptr, size, offset);
		pthread_mutex_unlock(dfd->_lock);
		if (op_ret == -1) {
			printf("Read Error %d\n", errno);
			op_errno = errno;
			goto out;
		}

		vec = (struct iovec *)malloc(sizeof(struct iovec));
		vec->iov_base = iobuf->ptr;
		vec->iov_len = op_ret;

		iobref = iobref_new();
		iobref_add(iobref, iobuf);

		isa_com_fdstat(this, dfd->_rfd->_fd, &stbuf);
out:
		printf("Readed data len is  %d\n", op_ret);
		//return
		STACK_UNWIND_STRICT (readv, frame, op_ret, op_errno,
								 vec, 1, &stbuf, iobref, NULL);
		if (iobref)
			iobref_unref (iobref);
		if (iobuf)
			iobuf_unref (iobuf);

	}
	return 0;
}

//call compress data function & call next translator writev
int com_isa_writev(call_frame_t *frame, xlator_t *this, fd_t *fd,
		struct iovec *vector, int32_t count, off_t offset, uint32_t flags,
		struct iobref *iobref, dict_t *xdata) {
	struct iatt            preop    = {0,};
	struct iatt            postop    = {0,};
	uint64_t tmpfd = 0;
	compress_fd_t *cfd = NULL;
	fd_ctx_get(fd, this, &tmpfd);
	cfd = (compress_fd_t *) (long)tmpfd;
	//compress_fd_t *cfd = get_fd_from_pool(fd);

	if (cfd == NULL) {
		printf("Write file error for file is not created\n");
		STACK_UNWIND_STRICT(writev, frame, -1, 0, NULL, NULL, NULL);
	} else {
		UINT32 i, sum = 0;
		printf("Write count %d len %d\n", count, vector[0].iov_len);
		for (i=0; i<count; i++) {
			sum += add_to_buffer(cfd, vector[i].iov_base, vector[i].iov_len);
		}
		printf("Write to  file offset  %d\n", offset);

		STACK_UNWIND_STRICT(writev, frame, sum, 0, &preop, &postop, NULL);
	}
	return 0;
}

int init(xlator_t *this) {
	//init buf
	init_inner_buffer();
	init_com_file_pool();
	init_decom_file_pool();
	com_isa_private_t *priv = NULL;
	if (!this->children) {
		gf_log("com-isa", GF_LOG_ERROR, "FATAL: com-isa should have no child");
		return -1;
	}
	if (!this->parents) {
		gf_log("this->name", GF_LOG_WARNING,
				"WARN: wrong volume config, check volume config file");
	}
	priv = GF_CALLOC(sizeof(com_isa_private_t), 1, 0);
	if (!priv)
		return -1;
	priv->com_isa_write = 1;
	priv->decom_isa_read = 1;

	data_t *data = NULL;

	data = dict_get(this->options, "com-isa-write");
	if (data) {
		if (gf_string2boolean(data->data, &priv->com_isa_write) == -1) {
			gf_log(this->name, GF_LOG_ERROR,
					"FATAL: config com-isa-write can only be boolean");
			return -1;
		}
	}
	data = dict_get(this->options, "com-isa-read");
	if (data) {
		if (gf_string2boolean(data->data, &priv->decom_isa_read) == -1) {
			gf_log(this->name, GF_LOG_ERROR,
					"FATAL: config decom-isa-read can only be boolean");
			return -1;
		}
	}
	this->private = priv;
	gf_log("com-isa", GF_LOG_DEBUG, "this tranlator loaded");
	return 0;
}

void fini(xlator_t *this) {
	com_isa_private_t *priv = this->private;
	if (!priv)
		return;
	this->private = NULL;
	GF_FREE(priv);

	return;
}
struct xlator_fops fops = { .readv = decom_isa_readv,
		.create = com_create, .writev = com_isa_writev, 
		.flush = com_flush, .open = com_open, .fstat = isa_com_fstat
		  };

struct xlator_cbks cbks;

struct volume_options options[] = { { .key = { "com-isa-write" }, .type =
		GF_OPTION_TYPE_BOOL }, { .key = { "decom-isa-read" }, .type =
		GF_OPTION_TYPE_BOOL }, { .key = { NULL } } };

