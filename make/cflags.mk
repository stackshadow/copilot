# CFLAGS are flags for the compiler

CFLAGS      += -I$(sourcePath)/
CFLAGS      += -I$(sourcePath)/src
CFLAGS      += -I$(sourcePath)/qt
CFLAGS      += -I$(sourcePath)/qt/ui
CFLAGS      += -I$(sourcePath)/qt/connections
#CFLAGS      += -I/usr/include/qt4/QtGui/
CFLAGS      += -I/usr/include/x86_64-linux-gnu/qt5/
CFLAGS      += -I$(targetPath)/include/evillib
CFLAGS      += -I$(sourcePath)/libs/evillib/core
CFLAGS      += -I$(sourcePath)/libs/evillib/extra

CFLAGS      += $(shell pkg-config Qt5Gui Qt5Widgets --cflags)
#CFLAGS      += $(shell pkg-config QtGui --cflags)

# flags for debugging-support
ifneq ($(DEBUG),)
CFLAGS      += -g
endif

# flags for library-build
ifneq ($(LIBRARY),)
CFLAGS      += -fPIC
endif
