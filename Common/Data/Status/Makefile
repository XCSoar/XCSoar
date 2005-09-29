# ==============================================================================
.SUFFIXES : .xcs

FILES=default.xcs template.xcs ../../Header/Status_defaults.cpp

# ------------------------------------------------------------------------------
build: $(FILES)

default.xcs : template.xcs
	cp template.xcs $@

../../Header/Status_defaults.cpp : default.xcs xcs2cpp.pl
	perl xcs2cpp.pl default.xcs > $@

