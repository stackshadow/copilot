#!/bin/make
#	buildPath=$(PWD)/build/evillib \

evillib.so:
	sourcePath=$(PWD)/libs/evillib \
	prefix=$(PWD)/target \
	make -C libs/evillib -f make/Makefile evillib-core-install

	sourcePath=$(PWD)/libs/evillib \
	prefix=$(PWD)/target \
	make -C libs/evillib -f make/Makefile evillib-core-dev-install



#	sourcePath=$(PWD)/libs/evillib \
#	make -C evillib -f make/Makefile evillib-extra


#	sourcePath=$(PWD)/libs/evillib \
#	prefix=$(PWD)/../target \
#	make -C evillib -f make/Makefile evillib-extra-install
