#!/bin/make

MAKE=make --no-print-directory --silent -f
MAKE=make --no-print-directory -f

include make/progs.mk
include make/sources.mk
include make/cflags.mk
include make/clibs.mk


include make/generic/bin.mk
include make/generic/qt.mk
include make/generic/gitversion.mk



premake:
#	$(eval test := $(shell find ./make/premake -type f -name "*.mk" -printf '%p '))
#	$(MAKE) make/Makefile $(test)
	@find ./make/premake -type f -name "*.mk" -print -exec \
	$(MAKE) {} debugflags premake \;
postmake:
	@find ./make/postmake -type f -name "*.mk" -exec \
	$(MAKE) {} postmake \;



binary: releaseflags premake binObjects binLinked postmake
binary-dbg: debugflags premake binObjects binLinked postmake
binary-qt: releaseflags premake qtUic qtObjects binObjects binLinked postmake
binary-qt-dbg: debugflags premake qtUic qtObjects binObjects binLinked postmake


library:
	@LIBRARY=1 $(MAKE) make/generic/library.mk library

library-dbg:
	@LIBRARY=1 DEBUG=1 $(MAKE) make/generic/library.mk library

clean: binClean qtClean
	@find ./make/postmake -type f -name "*.mk" -exec \
	$(MAKE) {} clean \;
	@find ./make/premake -type f -name "*.mk" -exec \
	$(MAKE) {} clean \;
	@$(MAKE) make/generic/qt.mk clean
	#@$(MAKE) make/generic/bin.mk clean
