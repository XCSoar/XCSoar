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

OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
ifeq ($(TARGET_IS_DARWIN),y)
# Use ANGLE on macOS (not iOS)
ifeq ($(TARGET_IS_IOS),y)
OPENGL_LDLIBS = -framework OpenGLES
else
# Include ANGLE configuration
include $(topdir)/build/angle.mk
OPENGL_CPPFLAGS += $(ANGLE_CPPFLAGS)
OPENGL_LDLIBS = $(ANGLE_LDLIBS)
endif
else
OPENGL_LDLIBS = -lGLESv2 -ldl
endif

OPENGL_CPPFLAGS += $(GLM_CPPFLAGS)

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
