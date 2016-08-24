# CFLAGS are flags for the compiler

### local stuff
CFLAGS      += -I$(sourcePath)/
CFLAGS      += -I$(sourcePath)/src
CFLAGS      += -I$(sourcePath)/qt
CFLAGS      += -I$(sourcePath)/qt/ui
CFLAGS      += -I$(sourcePath)/qt/connections
CFLAGS      += -I$(targetPath)/include/evillib
CFLAGS      += -I$(sourcePath)/libs/evillib/core
CFLAGS      += -I$(sourcePath)/libs/evillib/extra

### global stuff
CFLAGS      += -I/usr/include/x86_64-linux-gnu/qt5/
CFLAGS      += $(shell pkg-config Qt5Gui Qt5Widgets --cflags)
#CFLAGS      += $(shell pkg-config QtGui --cflags)
CFLAGS      += -I/usr/include/postgresql

# flags for debugging-support
ifneq ($(DEBUG),)
CFLAGS      += -g
endif

# flags for library-build
ifneq ($(LIBRARY),)
CFLAGS      += -fPIC
endif

ifeq ($(OS),Windows_NT)
CFLAGS      += -I/usr/win32/qt/include
CFLAGS      += -DET_BLOCKDEVICE_OFF

CFLAGS      += -pipe
CFLAGS      += -fno-keep-inline-dllexport
CFLAGS      += -std=gnu++11
CFLAGS      += -frtti
CFLAGS      += -Wall -Wextra
CFLAGS      += -fexceptions -mthreads
CFLAGS      += -DUNICODE -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_NEEDS_QMAIN

endif
