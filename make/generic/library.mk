#!/bin/make

# variables
include make/progs.mk
include make/sources.mk
include make/cflags.mk
include make/clibs.mk

# targets
include make/generic/objects.mk


sourcesLibRel       += $(subst ./,,$(sourcesLib))
sourcesLibFull      += $(addprefix $(sourcePath)/,$(sourcesLibRel))
objectsLibrary      += $(sourcesLibRel:.cpp=.o)
objectsLibraryFull  += $(addprefix $(buildPath)/,$(objectsLibrary))


library: $(objectsLibraryFull)


