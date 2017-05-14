#!/bin/make

include make/progs.mk
include make/sources.mk
include make/cflags.mk
include make/clibs.mk

utilVersion = -2.29.2
gitVersion = v2.29.2

CFLAGS       = -pie -fPIE -fPIC
CFLAGS      += -I$(sourcePath)/libs/util-linux$(utilVersion)/include
CFLAGS      += -I$(sourcePath)/libs/util-linux$(utilVersion)/libuuid/src
CFLAGS		+= -DHAVE_NANOSLEEP

sources		=
sources		+= libs/util-linux$(utilVersion)/lib/randutils.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/clear.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/copy.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/unpack.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/compare.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/pack.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/parse.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/gen_uuid.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/isnull.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/unparse.c
sources		+= libs/util-linux$(utilVersion)/libuuid/src/uuid_time.c

 	



include make/generic/bin.mk


.DEFAULT: uuid
.PHONY: uuid


$(sourcePath)/libs/util-linux$(utilVersion)/libuuid/src/gen_uuid.c:
	git clone --branch $(gitVersion) --depth 1 \
	git://git.kernel.org/pub/scm/utils/util-linux/util-linux.git \
	$(sourcePath)/libs/util-linux$(utilVersion)


premake: uuid
uuid: $(sourcePath)/libs/util-linux$(utilVersion)/libuuid/src/gen_uuid.c binObjects
clean: binClean
#test:
#	@echo "$(PWD)"
#	@$(MAKE) make/generic/bin.mk objects

