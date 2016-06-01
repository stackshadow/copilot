#!/bin/make

premake:
	find ./make/premake -type f -name "*.mk" -exec \
	make -f {} \;
postmake:
	find ./make/postmake -type f -name "*.mk" -exec \
	make -f {} \;

binary: premake
	make -f make/generic/bin.mk objects					# this create object-files
	make -f make/generic/bin.mk binary					# this link all together
	make -f make/Makefile postmake

binary-dbg: premake
	DEBUG=1 make -f make/generic/bin.mk objects
	DEBUG=1 make -f make/generic/bin.mk binary
	make -f make/Makefile postmake

binary-qt: premake
	QT=1 make -f make/generic/qt.mk uic					# this create object-files
	QT=1 make -f make/generic/qt.mk objects				# this create object-files
	QT=1 make -f make/generic/bin.mk objects			# this create object-files
	QT=1 make -f make/generic/bin.mk binary				# this link all together
	make -f make/Makefile postmake

binary-qt-dbg: premake
	QT=1 DEBUG=1 make -f make/generic/qt.mk uic			# this create object-files
	QT=1 DEBUG=1 make -f make/generic/qt.mk objects		# this create object-files
	QT=1 DEBUG=1 make -f make/generic/bin.mk objects	# this create object-files
	QT=1 DEBUG=1 make -f make/generic/bin.mk binary		# this link all together
	make -f make/Makefile postmake

library:
	LIBRARY=1 make -f make/generic/library.mk library

library-dbg:
	LIBRARY=1 DEBUG=1 make -f make/generic/library.mk library

clean:
	make -f make/generic/qt.mk clean
	make -f make/generic/bin.mk clean

