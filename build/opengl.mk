ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
OPENGL = y

  ifeq ($(EGL),y)
    # if our Android has EGL (2.3 or newer), we enable GLES2, too
    GLES2 = y
  else
    # old Androids use OpenGL ES 1.x only
    GLES = y
  endif

# the Kobo doesn't have OpenGL support
else ifeq ($(TARGET_IS_KOBO),y)
OPENGL = n
GLES = n

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
else
OPENGL_LDLIBS = -lGLESv2 -ldl
endif
else ifeq ($(GLES),y)
OPENGL_CPPFLAGS += -DHAVE_GLES -DHAVE_GLES1
OPENGL_LDLIBS = -lGLESv1_CM -ldl
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
