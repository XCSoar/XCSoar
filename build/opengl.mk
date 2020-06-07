ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
OPENGL = y
GLES = y

# the Kobo doesn't have OpenGL support
else ifeq ($(TARGET_IS_KOBO),y)
OPENGL = n
GLES = n

# the Raspberry Pi 4 uses EGL + GL/ES2
else ifeq ($(TARGET_IS_PI4),y)
OPENGL ?= y
GLES2 ?= y

# the Raspberry Pi uses EGL + GL/ES
else ifeq ($(TARGET_IS_PI),y)
OPENGL ?= y
GLES ?= y

# iOS uses GL/ES 2.0
else ifeq ($(TARGET_IS_IOS),y)
OPENGL ?= y
GLES2 ?= y

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

GLES2 ?= n
GLSL ?= $(GLES2)

ifeq ($(OPENGL),y)
OPENGL_CPPFLAGS = -DENABLE_OPENGL

ifeq ($(GLES2),y)
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES2
OPENGL_CPPFLAGS += $(GLM_CPPFLAGS)
ifeq ($(TARGET_IS_IOS),y)
OPENGL_LDLIBS = -framework OpenGLES
else ifeq ($(TARGET_IS_PI)$(TARGET_IS_PI4),yn)
OPENGL_LDLIBS = -lbrcmGLESv2 -ldl
else
OPENGL_LDLIBS = -lGLESv2 -ldl
endif
else ifeq ($(GLES),y)
OPENGL_CPPFLAGS += -DHAVE_GLES
ifeq ($(TARGET_IS_PI),y)
OPENGL_LDLIBS = -lbrcmGLESv2 -ldl
else
OPENGL_LDLIBS = -lGLESv1_CM -ldl
endif
else ifeq ($(TARGET_IS_DARWIN),y)
OPENGL_LDLIBS = -framework OpenGL
else
OPENGL_LDLIBS = -lGL
endif

ifeq ($(GLSL),y)
OPENGL_CPPFLAGS += -DUSE_GLSL
endif

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
