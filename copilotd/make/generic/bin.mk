#!/bin/make



sourcesRel      += $(subst ./,,$(sources))
sourcesFull     += $(addprefix $(sourcePath)/,$(sourcesRel))
objectsCPP       = $(sourcesRel:.cpp=.cpp.o)
objectsCC        = $(objectsCPP:.c=.c.o)
objects          = $(objectsCC)
objectsFull     += $(addprefix $(buildPath)/,$(objects))


# this target is empty for include
binDefault:
	@a=a


binObjects: $(objectsFull)

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

binLinked: $(objectsFull)
#	@$(ECHO) "LINK $@"
	$(CPP) \
	$(CFLAGS) \
	$(CLIBS) \
	$(shell find $(buildPath) -name "*.o") \
	-o $(buildPath)/app


binClean:
	@$(RM) $(objectsFull)

