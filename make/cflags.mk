# CFLAGS are flags for the compiler

### local stuff
CFLAGS      += -I$(sourcePath)/

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

# this target is empty for include
cflagsDummy:
	@a=a
	
debugflags:
	$(eval export CFLAGS += ${CFLAGSDBG} )

releaseflags:
	$(eval export CFLAGS += ${CFLAGSREL} )