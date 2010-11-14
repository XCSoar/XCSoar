# Build rules for the portable screen library

SCREEN_SRC_DIR = $(SRC)/Screen

SCREEN_SOURCES = \
	$(SCREEN_SRC_DIR)/ButtonWindow.cpp \
	$(SCREEN_SRC_DIR)/CheckBox.cpp \
	$(SCREEN_SRC_DIR)/ProgressBar.cpp \
	$(SCREEN_SRC_DIR)/RawBitmap.cpp \
	$(SCREEN_SRC_DIR)/Util.cpp \
	$(SCREEN_SRC_DIR)/Icon.cpp \
	$(SCREEN_SRC_DIR)/Brush.cpp \
	$(SCREEN_SRC_DIR)/Canvas.cpp \
	$(SCREEN_SRC_DIR)/Color.cpp \
	$(SCREEN_SRC_DIR)/VirtualCanvas.cpp \
	$(SCREEN_SRC_DIR)/BufferCanvas.cpp \
	$(SCREEN_SRC_DIR)/Font.cpp \
	$(SCREEN_SRC_DIR)/Pen.cpp \
	$(SCREEN_SRC_DIR)/Window.cpp \
	$(SCREEN_SRC_DIR)/WindowCanvas.cpp \
	$(SCREEN_SRC_DIR)/BufferWindow.cpp \
	$(SCREEN_SRC_DIR)/DoubleBufferWindow.cpp \
	$(SCREEN_SRC_DIR)/PaintWindow.cpp \
	$(SCREEN_SRC_DIR)/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/TextWindow.cpp \
	$(SCREEN_SRC_DIR)/EditWindow.cpp \
	$(SCREEN_SRC_DIR)/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/Dialog.cpp

ifeq ($(ENABLE_SDL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/SDL/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/SDL/Event.cpp \
	$(SCREEN_SRC_DIR)/SDL/Canvas.cpp \
	$(SCREEN_SRC_DIR)/SDL/Timer.cpp
ifeq ($(OPENGL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/OpenGL/Canvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Texture.cpp
endif
else
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/GDI/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/GDI/Event.cpp \
	$(SCREEN_SRC_DIR)/GDI/Canvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/BitmapCanvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/PaintCanvas.cpp
endif

SCREEN_OBJS = $(call SRC_TO_OBJ,$(SCREEN_SOURCES))

SCREEN_LIBS = $(TARGET_OUTPUT_DIR)/screen.a
SCREEN_LDLIBS = $(SDL_LDLIBS)
SCREEN_CPPFLAGS = $(SDL_CPPFLAGS)

ifeq ($(HAVE_WIN32),y)
ifeq ($(HAVE_CE),y)
SCREEN_LDLIBS += -lcommctrl
else
SCREEN_LDLIBS += -lcomctl32 -luser32 -lgdi32 -lmsimg32
endif
endif

$(SCREEN_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(SCREEN_LIBS): $(SCREEN_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
