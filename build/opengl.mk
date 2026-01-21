ifeq ($(TARGET),ANDROID)
# Android uses OpenGL/ES 2.0
OPENGL = y

# the Kobo doesn't have OpenGL support
else ifeq ($(TARGET_IS_KOBO),y)
OPENGL = n

# the Raspberry Pi uses EGL + GL/ES
else ifeq ($(TARGET_IS_PI),y)
OPENGL ?= y

# iOS uses GL/ES 2.0
else ifeq ($(TARGET_IS_IOS),y)
OPENGL ?= y

# macOS uses ANGLE (OpenGL ES via Metal backend)
else ifeq ($(TARGET_IS_DARWIN),y)
OPENGL ?= y

# the Cubieboard uses EGL + GL/ES
else ifeq ($(TARGET_IS_CUBIE),y)
OPENGL ?= y

# UNIX/Linux defaults to OpenGL
else ifeq ($(TARGET),UNIX)
OPENGL ?= y

else
# Windows defaults to GDI (no OpenGL)
OPENGL ?= n
endif

GLES2 ?= $(OPENGL)

ifeq ($(OPENGL),y)
OPENGL_CPPFLAGS = -DENABLE_OPENGL

ifeq ($(TARGET_IS_DARWIN),y)
# macOS/iOS always uses GLES API (ANGLE on macOS, native on iOS)
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
ifeq ($(TARGET_IS_IOS),y)
OPENGL_LDLIBS = -framework OpenGLES
else
# Include ANGLE configuration
include $(topdir)/build/angle.mk
OPENGL_CPPFLAGS += $(ANGLE_CPPFLAGS)
OPENGL_LDLIBS = $(ANGLE_LDLIBS)
endif
else ifeq ($(HAVE_WIN32),y)
ifeq ($(USE_ANGLE),y)
# Use ANGLE on Windows with SDL
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
include $(topdir)/build/angle.mk
OPENGL_CPPFLAGS += $(ANGLE_CPPFLAGS)
OPENGL_LDLIBS = $(ANGLE_LDLIBS)
else
# Use desktop OpenGL on Windows (WGL) without ANGLE
# glad is used to load OpenGL functions at runtime (opengl32.lib only exports GL 1.1)
OPENGL_CPPFLAGS += -isystem $(topdir)/lib/glad/include
OPENGL_LDLIBS = -lopengl32
GLAD_SOURCES = $(topdir)/lib/glad/src/glad.c
endif
else
# Other platforms (TODO: Actually, HAVE_GLES is wrongly defined for non-GLES targets such as GLX - but not used there)
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
OPENGL_LDLIBS = -lGLESv2 -ldl
endif

OPENGL_CPPFLAGS += $(GLM_CPPFLAGS)

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
