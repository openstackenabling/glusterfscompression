glusterfscompression
====================

Compression Translator for glusterFS

1) How to build?
   This translator need the ISA-L & zlib library. You can modify the Makefile, point the ISA-L library DIR, and the useful header files DIR. Also you need to point the GlusterFS library DIR, head DIR.

2) How to test?
   This translator is now experimental, Can not used in product environment. Only support write once read many times mode. Only for simple demo usage, you can use command "cp" test for reading & writing. Note that, you can only write once, can not rewrite or random write.
   You need to manual write the configuration volume file for test,  for local test, you can edit follow the example volume configuration file.

3) How to report issues?
   Some issues may exist in useing it, you can report it in github

