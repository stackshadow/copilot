#!/bin/make

MAKE=make --no-print-directory --silent -f

premake:
	@find ./make/premake -type f -name "*.mk" -exec \
	$(MAKE) {} \;
postmake:
	@find ./make/postmake -type f -name "*.mk" -exec \
	$(MAKE) {} \;

binary: premake
	@$(MAKE) make/generic/bin.mk objects					# this create object-files
	@$(MAKE) make/generic/bin.mk binary						# this link all together
	@$(MAKE) make/Makefile postmake

binary-dbg: premake
	@DEBUG=1 $(MAKE) make/generic/bin.mk objects
	@DEBUG=1 $(MAKE) make/generic/bin.mk binary
	@$(MAKE) make/Makefile postmake

binary-qt: premake
	@QT=1 $(MAKE) make/generic/qt.mk uic					# this create object-files
	@QT=1 $(MAKE) make/generic/qt.mk objects				# this create object-files
	@QT=1 $(MAKE) make/generic/bin.mk objects				# this create object-files
	@QT=1 $(MAKE) make/generic/bin.mk binary				# this link all together
	@$(MAKE) make/Makefile postmake

binary-qt-dbg: premake
	@QT=1 DEBUG=1 $(MAKE) make/generic/qt.mk uic			# this create object-files
	@QT=1 DEBUG=1 $(MAKE) make/generic/qt.mk objects		# this create object-files
	@QT=1 DEBUG=1 $(MAKE) make/generic/bin.mk objects		# this create object-files
	@QT=1 DEBUG=1 $(MAKE) make/generic/bin.mk binary		# this link all together
	@$(MAKE) make/Makefile postmake

library:
	@LIBRARY=1 $(MAKE) make/generic/library.mk library

library-dbg:
	@LIBRARY=1 DEBUG=1 $(MAKE) make/generic/library.mk library

clean:
	@$(MAKE) make/generic/qt.mk clean
	@$(MAKE) make/generic/bin.mk clean

