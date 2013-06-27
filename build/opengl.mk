ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
OPENGL = y
GLES = y

# the Kobo doesn't have OpenGL support
else ifeq ($(TARGET_IS_KOBO),y)
OPENGL = n
GLES = n

# the Raspberry Pi uses EGL + GL/ES
else ifeq ($(TARGET_IS_PI),y)
OPENGL ?= y
GLES ?= y

# the Cubieboard uses EGL + GL/ES
else ifeq ($(TARGET_HAS_MALI),y)
OPENGL ?= y
GLES ?= y

# UNIX/Linux defaults to OpenGL
else ifeq ($(TARGET),UNIX)
OPENGL ?= y
GLES ?= n
else
# Windows defaults to GDI (no OpenGL)
OPENGL ?= n
GLES = n
endif

ifeq ($(OPENGL),y)
OPENGL_CPPFLAGS = -DENABLE_OPENGL

ifeq ($(TARGET_IS_DARWIN),y)
OPENGL_LDLIBS = -framework OpenGL
else ifeq ($(GLES),y)
OPENGL_CPPFLAGS += -DHAVE_GLES
OPENGL_LDLIBS = -lGLESv1_CM -ldl
else
OPENGL_LDLIBS = -lGL
endif

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
