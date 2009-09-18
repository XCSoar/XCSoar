ENABLE_SDL := n

ifeq ($(ENABLE_SDL),y)
ifeq ($(TARGET),UNIX)
CPPFLAGS += $(shell pkg-config --cflags sdl)
LDLIBS += $(shell pkg-config --libs sdl)
else
CPPFLAGS += -I/usr/local/i586-mingw32msvc/include
LDLIBS += -L/usr/local/i586-mingw32msvc/lib
endif

CPPFLAGS += -DENABLE_SDL
LDLIBS += -lSDL_gfx -lSDL_ttf

OBJS += $(SRC)/Screen/Timer.o
else
OBJS += \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/PaintCanvas.o
endif
