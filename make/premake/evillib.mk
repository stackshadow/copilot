
include make/progs.mk
include make/sources.mk

.DEFAULT: evillib 

evillib: 

#evillib-core evillib-extra

evillib-core: $(targetPath)/lib/libevillib.so
$(targetPath)/lib/libevillib.so:
	sourcePath=$(PWD)/libs/evillib \
	prefix=$(targetPath) \
	make -C libs/evillib -f make/evillib-core.make library-install

	sourcePath=$(PWD)/libs/evillib \
	prefix=$(targetPath) \
	make -C libs/evillib -f make/evillib-core.make dev-install

evillib-extra: $(targetPath)/lib/libevillib-extra.so
$(targetPath)/lib/libevillib-extra.so:
	sourcePath=$(PWD)/libs/evillib \
	prefix=$(targetPath) \
	make -C libs/evillib -f make/evillib-extra.make library-install

	sourcePath=$(PWD)/libs/evillib \
	prefix=$(targetPath) \
	make -C libs/evillib -f make/evillib-extra.make dev-install
