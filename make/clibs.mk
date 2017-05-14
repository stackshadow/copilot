# CLIBS contains librarys which are linked to your binary / library
# note: you dont need to link the internal compiled library against our app
# this will be done internally

CLIBS       += -lstdc++

# libs for debugging-support
ifneq ($(DEBUG),)
CLIBS      +=
endif

# libs for library-build
ifneq ($(LIBRARY),)
CLIBS      +=
endif

ifeq ($(OS),Windows_NT)
CLIBS       += -L/usr/win32/qt/lib
CLIBS       += -Wl,-s -Wl,-subsystem,windows
CLIBS       += -mthreads
CLIBS       += -lglu32 -lopengl32 -lgdi32 -luser32 -lmingw32 -lshell32
CLIBS       += /usr/win32/qt/lib/libqtmain.a /usr/win32/qt/lib/libQt5Widgets.a /usr/win32/qt/lib/libQt5Gui.a /usr/win32/qt/lib/libQt5Core.a
endif
