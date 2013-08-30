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
# Change these to match your source code.
TARGET  = com-isa.so 
OBJECTS = com-isa.o com-common.o compress.o decompress.o 
 
# Change these to match your environment.
GLFS_SRC = /home/swift/glusterfs-3.4.0beta2
GLFS_LIB = /usr/lib
GLFS_VERS = 3.4.0beta2
HOST_OS  = GF_LINUX_HOST_OS
 
# You shouldn't need to change anything below here.
 
CFLAGS  = -shared -fPIC -g -Wall -O2 \
	-DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D$(HOST_OS) \
	-I$(GLFS_SRC) -I$(GLFS_SRC)/libglusterfs/src \
	-I$(GLFS_SRC)/contrib/uuid \
	-I/home/swift/isa-l_src_2.6/include \
	-I/usr/local/include
LDFLAGS = -shared -fPIC -nostartfiles -L$(GLFS_LIB) -L/usr/local/lib -lglusterfs -lpthread -lisa-l -lz
 
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS)  -o $(TARGET) 
clean:
	rm -f $(TARGET) $(OBJECTS)
install: $(TARGET)
	cp $(TARGET) $(GLFS_LIB)/glusterfs/$(GLFS_VERS)/xlator/isa/
	
