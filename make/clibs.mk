# CLIBS contains librarys which are linked to your binary / library
# note: you dont need to link the internal compiled library against our app
# this will be done internally

CLIBS       += -lstdc++
CLIBS       += $(shell pkg-config Qt5Gui Qt5Widgets --libs)
CLIBS       += -ljansson

#CLIBS       += -L$(targetPath)/lib
#CLIBS       += -Wl,-rpath $(targetPath)/lib
#CLIBS       += -levillib
#CLIBS       += -levillib-extra


CLIBS       += -lsqlite3
CLIBS       += -lpq

# libs for debugging-support
ifneq ($(DEBUG),)
CLIBS      +=
endif

# libs for library-build
ifneq ($(LIBRARY),)
CLIBS      +=
endif
