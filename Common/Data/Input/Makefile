FILES = ../../Header/InputEvents_defaults.cpp ../../Header/InputEvents_Text2Event.cpp

# ==============================================================================
build: $(FILES)

../../Header/InputEvents_defaults.cpp : default.xci xci2cpp.pl
	perl xci2cpp.pl default.xci > $@

../../Header/InputEvents_Text2Event.cpp : ../../Header/InputEvents.h h2cpp.pl
	perl h2cpp.pl > $@

