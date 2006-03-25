

FILES = \
	../../Header/InputEvents_defaults.cpp \
	../../Header/InputEvents_altair.cpp \
	../../Header/InputEvents_pc.cpp \
	../../Header/InputEvents_gnav.cpp \
	../../Header/InputEvents_Text2Event.cpp

# ==============================================================================
build: $(FILES)

../../Header/InputEvents_defaults.cpp : default.xci xci2cpp.pl
	perl xci2cpp.pl default.xci > $@

../../Header/InputEvents_pc.cpp : pc.xci xci2cpp.pl
	perl xci2cpp.pl pc.xci > $@

../../Header/InputEvents_altair.cpp : altair.xci xci2cpp.pl
	perl xci2cpp.pl altair.xci > $@

../../Header/InputEvents_gnav.cpp : gnav.xci xci2cpp.pl
	perl xci2cpp.pl gnav.xci > $@

../../Header/InputEvents_Text2Event.cpp : ../../Header/InputEvents.h h2cpp.pl
	perl h2cpp.pl > $@

