# Build rules for the portable screen library

USE_MEMORY_CANVAS ?= n

SCREEN_SRC_DIR = $(SRC)/Screen

SCREEN_SOURCES = \
	$(SCREEN_SRC_DIR)/Debug.cpp \
	$(SCREEN_SRC_DIR)/ProgressBar.cpp \
	$(SCREEN_SRC_DIR)/Util.cpp \
	$(SCREEN_SRC_DIR)/Icon.cpp \
	$(SCREEN_SRC_DIR)/Canvas.cpp \
	$(SCREEN_SRC_DIR)/Color.cpp \
	$(SCREEN_SRC_DIR)/BufferCanvas.cpp \
	$(SCREEN_SRC_DIR)/Window.cpp \
	$(SCREEN_SRC_DIR)/SolidContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/BufferWindow.cpp \
	$(SCREEN_SRC_DIR)/DoubleBufferWindow.cpp \
	$(SCREEN_SRC_DIR)/SingleWindow.cpp

SCREEN_CUSTOM_SOURCES = \
	$(SCREEN_SRC_DIR)/Custom/GeoBitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/Pen.cpp \
	$(SCREEN_SRC_DIR)/Custom/Timer.cpp \
	$(SCREEN_SRC_DIR)/Custom/LargeTextWindow.cpp \
	$(SCREEN_SRC_DIR)/Custom/Window.cpp \
	$(SCREEN_SRC_DIR)/Custom/WList.cpp \
	$(SCREEN_SRC_DIR)/Custom/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/Custom/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/Custom/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/Custom/MoreCanvas.cpp

ifeq ($(COREGRAPHICS),y)
SCREEN_CUSTOM_SOURCES_IMG = \
	$(SCREEN_SRC_DIR)/Apple/ImageDecoder.cpp
endif

ifeq ($(LIBPNG),y)
SCREEN_CUSTOM_SOURCES_IMG += $(SCREEN_SRC_DIR)/Custom/LibPNG.cpp
endif

ifeq ($(LIBJPEG),y)
SCREEN_CUSTOM_SOURCES_IMG += $(SCREEN_SRC_DIR)/Custom/LibJPEG.cpp
endif

ifeq ($(TIFF),y)
SCREEN_CUSTOM_SOURCES_IMG += $(SCREEN_SRC_DIR)/Custom/LibTiff.cpp
endif

ifeq ($(TARGET),ANDROID)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(SCREEN_SRC_DIR)/Android/Window.cpp \
	$(SCREEN_SRC_DIR)/Android/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/Android/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/Android/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/Android/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Android/Font.cpp
ifeq ($(TIFF),y)
SCREEN_SOURCES += $(SCREEN_SRC_DIR)/Custom/LibTiff.cpp
endif
endif

ifeq ($(DITHER),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Memory/Dither.cpp
endif

ifeq ($(FREETYPE),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/FreeType/Font.cpp \
	$(SCREEN_SRC_DIR)/FreeType/Init.cpp
endif

ifeq ($(call bool_or,$(APPKIT),$(UIKIT)),y)
SCREEN_SOURCES += $(SCREEN_SRC_DIR)/Apple/Font.cpp
endif

ifeq ($(USE_X11),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/X11/TopWindow.cpp
endif

ifeq ($(USE_WAYLAND),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Wayland/TopWindow.cpp
endif

ifeq ($(OPENGL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Custom/Cache.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Init.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Dynamic.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Rotate.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Geo.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Globals.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Extension.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/FBO.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/VertexArray.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/ConstantAlpha.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/RawBitmap.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Canvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/BufferCanvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/SubCanvas.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Texture.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/UncompressedImage.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Buffer.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Shapes.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Surface.cpp \
	$(SCREEN_SRC_DIR)/OpenGL/Triangulate.cpp

ifeq ($(GLSL),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/OpenGL/Shaders.cpp
endif
endif

ifeq ($(ENABLE_SDL),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Custom/Files.cpp \
	$(SCREEN_SRC_DIR)/Custom/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/SDL/Window.cpp \
	$(SCREEN_SRC_DIR)/SDL/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/SDL/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/SDL/Init.cpp
ifeq ($(OPENGL),n)
USE_MEMORY_CANVAS = y
endif
else ifeq ($(EGL)$(TARGET_IS_ANDROID),yn)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(SCREEN_SRC_DIR)/Custom/Files.cpp \
	$(SCREEN_SRC_DIR)/Custom/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/TTY/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/EGL/Init.cpp \
	$(SCREEN_SRC_DIR)/EGL/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/FB/Window.cpp \
	$(SCREEN_SRC_DIR)/FB/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/SingleWindow.cpp
else ifeq ($(GLX),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(SCREEN_SRC_DIR)/Custom/Files.cpp \
	$(SCREEN_SRC_DIR)/Custom/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/GLX/Init.cpp \
	$(SCREEN_SRC_DIR)/GLX/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/FB/Window.cpp \
	$(SCREEN_SRC_DIR)/FB/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/SingleWindow.cpp
else ifeq ($(VFB),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(SCREEN_SRC_DIR)/Custom/Files.cpp \
	$(SCREEN_SRC_DIR)/Custom/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/FB/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/FB/Window.cpp \
	$(SCREEN_SRC_DIR)/FB/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/Init.cpp
FB_CPPFLAGS = -DUSE_VFB
else ifeq ($(USE_FB),y)
SCREEN_SOURCES += $(SCREEN_CUSTOM_SOURCES_IMG)
SCREEN_SOURCES += \
	$(SCREEN_CUSTOM_SOURCES) \
	$(SCREEN_SRC_DIR)/Custom/Files.cpp \
	$(SCREEN_SRC_DIR)/Custom/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Custom/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/Memory/Export.cpp \
	$(SCREEN_SRC_DIR)/TTY/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/FB/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/TopCanvas.cpp \
	$(SCREEN_SRC_DIR)/FB/Window.cpp \
	$(SCREEN_SRC_DIR)/FB/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/FB/Init.cpp
FB_CPPFLAGS = -DUSE_FB
else ifeq ($(HAVE_WIN32),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/GDI/WindowCanvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/VirtualCanvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/Init.cpp \
	$(SCREEN_SRC_DIR)/GDI/Font.cpp \
	$(SCREEN_SRC_DIR)/GDI/Timer.cpp \
	$(SCREEN_SRC_DIR)/GDI/Window.cpp \
	$(SCREEN_SRC_DIR)/GDI/PaintWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/ContainerWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/LargeTextWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/SingleWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/TopWindow.cpp \
	$(SCREEN_SRC_DIR)/GDI/Pen.cpp \
	$(SCREEN_SRC_DIR)/GDI/Brush.cpp \
	$(SCREEN_SRC_DIR)/GDI/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/GDI/ResourceBitmap.cpp \
	$(SCREEN_SRC_DIR)/GDI/RawBitmap.cpp \
	$(SCREEN_SRC_DIR)/GDI/Canvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/BufferCanvas.cpp \
	$(SCREEN_SRC_DIR)/GDI/PaintCanvas.cpp
GDI_CPPFLAGS = -DUSE_GDI
GDI_CPPFLAGS += -DUSE_WINUSER
GDI_LDLIBS = -luser32 -lgdi32 -lmsimg32

ifeq ($(TARGET),PC)
GDI_LDLIBS += -Wl,-subsystem,windows
endif
endif

ifeq ($(USE_MEMORY_CANVAS),y)
SCREEN_SOURCES += \
	$(SCREEN_SRC_DIR)/Custom/Cache.cpp \
	$(SCREEN_SRC_DIR)/Memory/Bitmap.cpp \
	$(SCREEN_SRC_DIR)/Memory/RawBitmap.cpp \
	$(SCREEN_SRC_DIR)/Memory/VirtualCanvas.cpp \
	$(SCREEN_SRC_DIR)/Memory/SubCanvas.cpp \
	$(SCREEN_SRC_DIR)/Memory/Canvas.cpp
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
