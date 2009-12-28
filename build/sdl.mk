ENABLE_SDL := n

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
CPPFLAGS += $(shell pkg-config --cflags sdl)
LDLIBS += $(shell pkg-config --libs sdl)
else
CPPFLAGS += -I/usr/local/i586-mingw32msvc/include
LDLIBS += -L/usr/local/i586-mingw32msvc/lib -lSDL
endif

CPPFLAGS += -DENABLE_SDL
LDLIBS += -lSDL_gfx -lSDL_ttf

XCSOAR_SOURCES += $(SRC)/Screen/Timer.cpp
else
XCSOAR_SOURCES += \
	$(SRC)/Screen/BufferCanvas.cpp \
	$(SRC)/Screen/PaintCanvas.cpp
endif
