ifeq ($(TARGET),ANDROID)
# Android uses OpenGL/ES 2.0
OPENGL = y
GLES2 = y

# the Kobo doesn't have OpenGL support
else ifeq ($(TARGET_IS_KOBO),y)
OPENGL = n

# the Raspberry Pi uses EGL + GL/ES
else ifeq ($(TARGET_IS_PI),y)
OPENGL ?= y
GLES2 ?= y

# iOS uses GL/ES 2.0
else ifeq ($(TARGET_IS_IOS),y)
OPENGL ?= y
GLES2 ?= y

# the Cubieboard uses EGL + GL/ES
else ifeq ($(TARGET_HAS_MALI),y)
OPENGL ?= y
GLES2 ?= y
# UNIX/Linux defaults to OpenGL
else ifeq ($(TARGET),UNIX)
OPENGL ?= y

else
# Windows defaults to GDI (no OpenGL)
OPENGL ?= n
endif

GLES2 ?= n

ifeq ($(OPENGL),y)
OPENGL_CPPFLAGS = -DENABLE_OPENGL

ifeq ($(GLES2),y)
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
ifeq ($(TARGET_IS_IOS),y)
OPENGL_LDLIBS = -framework OpenGLES
else ifeq ($(TARGET_IS_PI),y)
OPENGL_LDLIBS = -lbrcmGLESv2 -ldl
else
OPENGL_LDLIBS = -lGLESv2 -ldl
endif
else ifeq ($(TARGET_IS_DARWIN),y)
OPENGL_LDLIBS = -framework OpenGL
else
OPENGL_LDLIBS = -lGL
endif

OPENGL_CPPFLAGS += $(GLM_CPPFLAGS)

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
