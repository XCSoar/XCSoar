# Build rules for the portable screen library

SCREEN_SRC_DIR = $(SRC)/Screen

SCREEN_SOURCES = \
	$(SCREEN_SRC_DIR)/Debug.cpp \
	$(SCREEN_SRC_DIR)/ProgressBar.cpp \
	$(SCREEN_SRC_DIR)/RawBitmap.cpp \
	$(SCREEN_SRC_DIR)/Util.cpp \
	$(SCREEN_SRC_DIR)/Icon.cpp \
	$(SCREEN_SRC_DIR)/Brush.cpp \
	$(SCREEN_SRC_DIR)/Canvas.cpp \
	$(SCREEN_SRC_DIR)/Color.cpp \
	$(SCREEN_SRC_DIR)/BufferCanvas.cpp \
	$(SCREEN_SRC_DIR)/Pen.cpp \
	$(SCREEN_SRC_DIR)/Window.cpp \
	$(SCREEN_SRC_DIR)/BufferWindow.cpp \
	$(SCREEN_SRC_DIR)/DoubleBufferWindow.cpp \
	$(SCREEN_SRC_DIR)/PaintWindow.cpp \
	$(SCREEN_SRC_DIR)/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/TextWindow.cpp \
	$(SCREEN_SRC_DIR)/SingleWindow.cpp

ifeq ($(ENABLE_SDL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/SDL/Window.cpp \
	$(SCREEN_SRC_DIR)/SDL/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/ButtonWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/CheckBox.cpp \
	$(SCREEN_SRC_DIR)/SDL/EditWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/SDL/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/Canvas.cpp
ifeq ($(TARGET),ANDROID)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Android/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/Android/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/Android/Font.cpp \
	$(SCREEN_SRC_DIR)/Android/Event.cpp
else
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/SDL/Init.cpp \
	$(SCREEN_SRC_DIR)/SDL/Font.cpp \
	$(SCREEN_SRC_DIR)/SDL/Event.cpp \
	$(SCREEN_SRC_DIR)/SDL/Timer.cpp
endif
ifeq ($(OPENGL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/OpenGL/Init.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Globals.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Extension.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/VertexArray.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Cache.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Canvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Texture.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Surface.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Triangulate.cpp
else
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/SDL/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/VirtualCanvas.cpp \
	$(SCREEN_SRC_DIR)/WindowCanvas.cpp
endif
else
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/VirtualCanvas.cpp \
	$(SCREEN_SRC_DIR)/WindowCanvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/Init.cpp \
	$(SCREEN_SRC_DIR)/GDI/Font.cpp \
	$(SCREEN_SRC_DIR)/GDI/AlphaBlend.cpp \
	$(SCREEN_SRC_DIR)/GDI/Window.cpp \
	$(SCREEN_SRC_DIR)/GDI/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/ButtonWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/EditWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/GDI/Event.cpp \
	$(SCREEN_SRC_DIR)/GDI/Canvas.cpp \
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

ifeq ($(TARGET),PC)
SCREEN_LDLIBS += -Wl,-subsystem,windows
endif

$(SCREEN_OBJS): CPPFLAGS += $(SCREEN_CPPFLAGS)
$(SCREEN_LIBS): $(SCREEN_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
