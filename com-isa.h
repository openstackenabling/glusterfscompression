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

#ifndef __COM_ISA_H__
#define __COM_ISA_H__

#ifndef _CONFIG_H
#define _CONFIG_H
#include "config.h"
#endif

#include "com-common.h"


typedef struct {
	gf_boolean_t com_isa_write;
	gf_boolean_t decom_isa_read;
} com_isa_private_t;

#endif /** end for com_isa_h**/
