# Build rules for the portable screen library

USE_MEMORY_CANVAS ?= n

SCREEN_SRC_DIR = $(SRC)/Screen
CANVAS_SRC_DIR = $(SRC)/ui/canvas
WINDOW_SRC_DIR = $(SRC)/ui/window

SCREEN_SOURCES = \
	$(SCREEN_SRC_DIR)/Debug.cpp \
	$(SCREEN_SRC_DIR)/ProgressBar.cpp \
	$(CANVAS_SRC_DIR)/Ramp.cpp \
	$(CANVAS_SRC_DIR)/Util.cpp \
	$(CANVAS_SRC_DIR)/Font.cpp \
	$(CANVAS_SRC_DIR)/Icon.cpp \
	$(CANVAS_SRC_DIR)/Canvas.cpp \
	$(CANVAS_SRC_DIR)/Color.cpp \
	$(CANVAS_SRC_DIR)/BufferCanvas.cpp \
	$(WINDOW_SRC_DIR)/Window.cpp \
	$(WINDOW_SRC_DIR)/SolidContainerWindow.cpp \
	$(WINDOW_SRC_DIR)/BufferWindow.cpp \
	$(WINDOW_SRC_DIR)/DoubleBufferWindow.cpp \
	$(WINDOW_SRC_DIR)/SingleWindow.cpp

SCREEN_CUSTOM_SOURCES = \
	$(WINDOW_SRC_DIR)/custom/DoubleClick.cpp \
	$(CANVAS_SRC_DIR)/custom/GeoBitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/Pen.cpp \
	$(SCREEN_SRC_DIR)/Custom/LargeTextWindow.cpp \
	$(WINDOW_SRC_DIR)/custom/Window.cpp \
	$(WINDOW_SRC_DIR)/custom/WList.cpp \
	$(WINDOW_SRC_DIR)/custom/ContainerWindow.cpp \
	$(WINDOW_SRC_DIR)/custom/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/custom/SingleWindow.cpp \
	$(CANVAS_SRC_DIR)/custom/MoreCanvas.cpp

ifeq ($(COREGRAPHICS),y)
SCREEN_CUSTOM_SOURCES_IMG = \
	$(CANVAS_SRC_DIR)/apple/ImageDecoder.cpp
endif

ifeq ($(LIBPNG),y)
SCREEN_CUSTOM_SOURCES_IMG += $(CANVAS_SRC_DIR)/custom/LibPNG.cpp
endif

ifeq ($(LIBJPEG),y)
SCREEN_CUSTOM_SOURCES_IMG += $(CANVAS_SRC_DIR)/custom/LibJPEG.cpp
endif

ifeq ($(TIFF),y)
SCREEN_CUSTOM_SOURCES_IMG += $(CANVAS_SRC_DIR)/custom/LibTiff.cpp
endif

ifeq ($(TARGET),ANDROID)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(WINDOW_SRC_DIR)/android/Window.cpp \
	$(WINDOW_SRC_DIR)/android/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/android/SingleWindow.cpp \
	$(CANVAS_SRC_DIR)/android/TopCanvas.cpp \
	$(CANVAS_SRC_DIR)/android/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/android/Font.cpp
ifeq ($(TIFF),y)
SCREEN_SOURCES += $(CANVAS_SRC_DIR)/custom/LibTiff.cpp
endif
endif

ifeq ($(DITHER),y)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/memory/Dither.cpp
endif

ifeq ($(FREETYPE),y)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/freetype/Font.cpp \
	$(CANVAS_SRC_DIR)/freetype/Init.cpp
endif

ifeq ($(call bool_or,$(APPKIT),$(UIKIT)),y)
SCREEN_SOURCES += $(CANVAS_SRC_DIR)/apple/Font.cpp
endif

ifeq ($(USE_X11),y)
SCREEN_SOURCES += \
	$(WINDOW_SRC_DIR)/x11/TopWindow.cpp
endif

ifeq ($(USE_WAYLAND),y)
SCREEN_SOURCES += \
	$(WINDOW_SRC_DIR)/wayland/TopWindow.cpp
endif

ifeq ($(OPENGL),y)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/custom/Cache.cpp \
	$(CANVAS_SRC_DIR)/opengl/Init.cpp \
	$(CANVAS_SRC_DIR)/opengl/Dynamic.cpp \
	$(CANVAS_SRC_DIR)/opengl/Rotate.cpp \
	$(CANVAS_SRC_DIR)/opengl/Geo.cpp \
	$(CANVAS_SRC_DIR)/opengl/Globals.cpp \
	$(CANVAS_SRC_DIR)/opengl/Extension.cpp \
	$(CANVAS_SRC_DIR)/opengl/FBO.cpp \
	$(CANVAS_SRC_DIR)/opengl/VertexArray.cpp \
	$(CANVAS_SRC_DIR)/opengl/ConstantAlpha.cpp \
	$(CANVAS_SRC_DIR)/opengl/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/opengl/RawBitmap.cpp \
	$(CANVAS_SRC_DIR)/opengl/Canvas.cpp \
	$(CANVAS_SRC_DIR)/opengl/BufferCanvas.cpp \
	$(CANVAS_SRC_DIR)/opengl/TopCanvas.cpp \
	$(CANVAS_SRC_DIR)/opengl/SubCanvas.cpp \
	$(CANVAS_SRC_DIR)/opengl/Texture.cpp \
	$(CANVAS_SRC_DIR)/opengl/UncompressedImage.cpp \
	$(CANVAS_SRC_DIR)/opengl/Buffer.cpp \
	$(CANVAS_SRC_DIR)/opengl/Shapes.cpp \
	$(CANVAS_SRC_DIR)/opengl/Surface.cpp \
	$(CANVAS_SRC_DIR)/opengl/Shaders.cpp \
	$(CANVAS_SRC_DIR)/opengl/Triangulate.cpp
endif

ifeq ($(ENABLE_SDL),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/custom/Files.cpp \
	$(CANVAS_SRC_DIR)/custom/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/ResourceBitmap.cpp \
	$(CANVAS_SRC_DIR)/sdl/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/sdl/Window.cpp \
	$(WINDOW_SRC_DIR)/sdl/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/sdl/SingleWindow.cpp \
	$(WINDOW_SRC_DIR)/sdl/Init.cpp
ifeq ($(OPENGL),n)
USE_MEMORY_CANVAS = y
endif
else ifeq ($(EGL)$(TARGET_IS_ANDROID),yn)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(CANVAS_SRC_DIR)/custom/Files.cpp \
	$(CANVAS_SRC_DIR)/custom/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/ResourceBitmap.cpp \
	$(CANVAS_SRC_DIR)/tty/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/egl/Init.cpp \
	$(CANVAS_SRC_DIR)/egl/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/fb/Window.cpp \
	$(WINDOW_SRC_DIR)/fb/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/fb/SingleWindow.cpp
else ifeq ($(GLX),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(CANVAS_SRC_DIR)/custom/Files.cpp \
	$(CANVAS_SRC_DIR)/custom/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/ResourceBitmap.cpp \
	$(WINDOW_SRC_DIR)/glx/Init.cpp \
	$(CANVAS_SRC_DIR)/glx/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/fb/Window.cpp \
	$(WINDOW_SRC_DIR)/fb/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/fb/SingleWindow.cpp
else ifeq ($(VFB),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(CANVAS_SRC_DIR)/custom/Files.cpp \
	$(CANVAS_SRC_DIR)/custom/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/ResourceBitmap.cpp \
	$(CANVAS_SRC_DIR)/fb/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/fb/Window.cpp \
	$(WINDOW_SRC_DIR)/fb/TopWindow.cpp \
	$(WINDOW_SRC_DIR)/fb/SingleWindow.cpp \
	$(WINDOW_SRC_DIR)/fb/Init.cpp
FB_CPPFLAGS = -DUSE_VFB
else ifeq ($(USE_FB),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(CANVAS_SRC_DIR)/custom/Files.cpp \
	$(CANVAS_SRC_DIR)/custom/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/custom/ResourceBitmap.cpp \
	$(CANVAS_SRC_DIR)/memory/Export.cpp \
	$(CANVAS_SRC_DIR)/tty/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/fb/TopWindow.cpp \
	$(CANVAS_SRC_DIR)/fb/TopCanvas.cpp \
	$(WINDOW_SRC_DIR)/fb/Window.cpp \
	$(WINDOW_SRC_DIR)/fb/SingleWindow.cpp \
	$(WINDOW_SRC_DIR)/fb/Init.cpp
FB_CPPFLAGS = -DUSE_FB
else ifeq ($(HAVE_WIN32),y)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/gdi/WindowCanvas.cpp \
	$(CANVAS_SRC_DIR)/gdi/VirtualCanvas.cpp \
	$(CANVAS_SRC_DIR)/gdi/Font.cpp \
	$(WINDOW_SRC_DIR)/gdi/Init.cpp \
	$(WINDOW_SRC_DIR)/gdi/Window.cpp \
	$(WINDOW_SRC_DIR)/gdi/PaintWindow.cpp \
	$(WINDOW_SRC_DIR)/gdi/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/LargeTextWindow.cpp \
	$(WINDOW_SRC_DIR)/gdi/SingleWindow.cpp \
	$(WINDOW_SRC_DIR)/gdi/TopWindow.cpp \
	$(CANVAS_SRC_DIR)/gdi/Pen.cpp \
	$(CANVAS_SRC_DIR)/gdi/Brush.cpp \
	$(CANVAS_SRC_DIR)/gdi/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/gdi/GdiPlusBitmap.cpp \
	$(CANVAS_SRC_DIR)/gdi/ResourceBitmap.cpp \
	$(CANVAS_SRC_DIR)/gdi/RawBitmap.cpp \
	$(CANVAS_SRC_DIR)/gdi/Canvas.cpp \
	$(CANVAS_SRC_DIR)/gdi/BufferCanvas.cpp \
	$(CANVAS_SRC_DIR)/gdi/PaintCanvas.cpp
GDI_CPPFLAGS = -DUSE_GDI
GDI_CPPFLAGS += -DUSE_WINUSER
GDI_LDLIBS = -luser32 -lgdi32 -lmsimg32 -lgdiplus

ifeq ($(TARGET),PC)
GDI_LDLIBS += -Wl,-subsystem,windows
endif
endif

ifeq ($(USE_MEMORY_CANVAS),y)
SCREEN_SOURCES += \
	$(CANVAS_SRC_DIR)/custom/Cache.cpp \
	$(CANVAS_SRC_DIR)/memory/Bitmap.cpp \
	$(CANVAS_SRC_DIR)/memory/RawBitmap.cpp \
	$(CANVAS_SRC_DIR)/memory/VirtualCanvas.cpp \
	$(CANVAS_SRC_DIR)/memory/SubCanvas.cpp \
	$(CANVAS_SRC_DIR)/memory/Canvas.cpp
MEMORY_CANVAS_CPPFLAGS = -DUSE_MEMORY_CANVAS
endif

SCREEN_CPPFLAGS_INTERNAL = \
	$(FREETYPE_CPPFLAGS) \
	$(LIBPNG_CPPFLAGS) \
	$(LIBJPEG_CPPFLAGS) \
	$(LIBTIFF_CPPFLAGS) \
	$(COREGRAPHICS_CPPFLAGS)

SCREEN_CPPFLAGS = \
	$(LINUX_INPUT_CPPFLAGS) \
	$(LIBINPUT_CPPFLAGS) \
	$(SDL_CPPFLAGS) \
	$(GDI_CPPFLAGS) \
	$(FREETYPE_FEATURE_CPPFLAGS) \
	$(APPKIT_CPPFLAGS) \
	$(UIKIT_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) \
	$(WAYLAND_CPPFLAGS) \
	$(EGL_CPPFLAGS) \
	$(EGL_FEATURE_CPPFLAGS) \
	$(GLX_CPPFLAGS) \
	$(POLL_EVENT_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

SCREEN_LDLIBS = \
	$(SDL_LDLIBS) \
	$(GDI_LDLIBS) \
	$(OPENGL_LDLIBS) \
	$(FREETYPE_LDLIBS) \
	$(LIBPNG_LDLIBS) $(LIBJPEG_LDLIBS) \
	$(LIBTIFF_LDLIBS) \
	$(WAYLAND_LDLIBS) \
	$(EGL_LDLIBS) \
	$(GLX_LDLIBS) \
	$(FB_LDLIBS) \
	$(COREGRAPHICS_LDLIBS) \
	$(APPKIT_LDLIBS) \
	$(UIKIT_LDLIBS)

$(eval $(call link-library,screen,SCREEN))

SCREEN_LDADD += \
	$(SDL_LDADD) \
	$(FB_LDADD) \
	$(FREETYPE_LDADD) \
	$(LIBPNG_LDADD) $(LIBJPEG_LDADD)

ifeq ($(USE_FB)$(VFB),yy)
$(error USE_FB and VFB are mutually exclusive)
endif

ifeq ($(USE_FB)$(EGL),yy)
$(error USE_FB and EGL are mutually exclusive)
endif

ifeq ($(USE_FB)$(GLX),yy)
$(error USE_FB and GLX are mutually exclusive)
endif

ifeq ($(USE_FB)$(ENABLE_SDL),yy)
$(error USE_FB and SDL are mutually exclusive)
endif

ifeq ($(VFB)$(EGL),yy)
$(error VFB and EGL are mutually exclusive)
endif

ifeq ($(VFB)$(GLX),yy)
$(error VFB and GLX are mutually exclusive)
endif

ifeq ($(VFB)$(ENABLE_SDL),yy)
$(error VFB and SDL are mutually exclusive)
endif

ifeq ($(EGL)$(ENABLE_SDL),yy)
$(error EGL and SDL are mutually exclusive)
endif

ifeq ($(GLX)$(ENABLE_SDL),yy)
$(error GLX and SDL are mutually exclusive)
endif

ifeq ($(EGL)$(OPENGL),yn)
$(error EGL requires OpenGL)
endif

ifeq ($(GLX)$(OPENGL),yn)
$(error GLX requires OpenGL)
endif

ifeq ($(USE_MEMORY_CANVAS)$(OPENGL),yy)
$(error MemoryCanvas and OpenGL are mutually exclusive)
endif
