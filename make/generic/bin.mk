#!/bin/make

# variables
include make/progs.mk
include make/sources.mk
include make/cflags.mk
include make/clibs.mk


sourcesRel      += $(subst ./,,$(sources))
sourcesFull     += $(addprefix $(sourcePath)/,$(sourcesRel))
objectsCPP       = $(sourcesRel:.cpp=.cpp.o)
objectsCC        = $(objectsCPP:.c=.c.o)
objects          = $(objectsCC)
objectsFull     += $(addprefix $(buildPath)/,$(objects))


objects: $(objectsFull)

$(buildPath)/%.cpp.o: $(sourcePath)/%.cpp
	@$(ECHO) "CC    $@"
	@$(MKDIR) $(shell dirname $@)
	$(CPP) $(CFLAGS) \
	-c $< -o $@
	@$(ECHO) ""

$(buildPath)/%.c.o: $(sourcePath)/%.c
	@$(ECHO) "CC    $@"
	@$(MKDIR) $(shell dirname $@)
	$(CC) $(CFLAGS) \
	-c $< -o $@
	@$(ECHO) ""

binary: $(objectsFull)
	@$(ECHO) "LD    $@"
	$(CPP) \
	$(CFLAGS) \
	$(CLIBS) \
	$(shell find $(buildPath) -name "*.o") \
	-o $(buildPath)/app


clean:
	@$(RM) $(objectsFull)

