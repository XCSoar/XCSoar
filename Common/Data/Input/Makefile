FILES = ../../Header/InputEvents_keys.h

# ==============================================================================
build: $(FILES)

%.h : template.xci xci2cpp.pl
	perl xci2cpp.pl template.xci > $@

