#!/bin/make


# every cpp file needs to be compiled in an moc.cpp and in an qt.o
sourcesQTRel        += $(subst ./,,$(sourcesQT))
sourcesQTFull       += $(addprefix $(sourcePath)/,$(sourcesQTRel))

sourcesQTMocRel     += $(sourcesQTRel:.cpp=.moc.cpp)
sourcesQTMocFull    += $(addprefix $(sourcePath)/,$(sourcesQTMocRel))

# every .cpp -> qt.o
# every moc.cpp -> moc.o
objectsQTRel        += $(sourcesQTRel:.cpp=.qt.o)
objectsQTRel        += $(sourcesQTMocRel:.moc.cpp=.moc.o)
objectsQTFull       += $(addprefix $(buildPath)/,$(objectsQTRel))


sourcesUiRel        =  $(subst ./,,$(sourcesUI))
headersUiRel        =  $(sourcesUiRel:.ui=.ui.h)
headersUiFull       += $(addprefix $(sourcePath),$(headersUiRel))

# this target is empty for include
qtDefault:
	@a=a

qtObjects: $(objectsQTFull)

$(sourcePath)/%.moc.cpp: $(sourcePath)/%.h
	@$(ECHO) "MOC    $@"
	@$(MKDIR) $(shell dirname $@)
	$(MOC) $< > $@


$(buildPath)/%.moc.o: $(sourcePath)/%.moc.cpp
	@$(ECHO) "CC-QT    $@"
	@$(MKDIR) $(shell dirname $@)
	$(CPP) $(CFLAGS) -fPIC -x c++ -c $< -o $@
	@$(ECHO) ""

qtUic: $(headersUiFull)
%.ui.h: %.ui
	$(UIC) \
	-g cpp \
	$< > $@

$(buildPath)/%.qt.o: $(sourcePath)/%.cpp
	@$(ECHO) "CC-QT    $@"
	@$(MKDIR) $(shell dirname $@)
	$(CPP) $(CFLAGS) -fPIC -x c++ -c $< -o $@
	@$(ECHO) ""



qtClean:
	@$(ECHO) "QT CLEAN"
	@$(RM) $(objectsQTFull)
	@$(RM) $(sourcesQTMocFull)
	@$(RM) $(headersUiFull)



