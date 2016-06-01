

# variables
include make/progs.mk
include make/sources.mk
include make/cflags.mk
include make/clibs.mk

.DEFAULT: $(targetPath)/bin/dodb

dodb: $(targetPath)/bin $(targetPath)/bin/dodb

$(targetPath)/bin/dodb: $(buildPath)/app
	$(CP) $< $@

$(targetPath)/bin:
	$(MKDIR) $@

