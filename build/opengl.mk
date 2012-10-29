ifeq ($(TARGET),ANDROID)
# Android must use OpenGL
OPENGL = y
# UNIX/Linux defaults to OpenGL, but can use SDL_gfx instead
else ifeq ($(TARGET),UNIX)
OPENGL ?= y
else
# Windows defaults to GDI (no OpenGL)
OPENGL ?= n
endif

ifeq ($(OPENGL),y)
OPENGL_CPPFLAGS = -DENABLE_OPENGL

ifeq ($(TARGET_IS_DARWIN),y)
OPENGL_LDLIBS = -framework OpenGL
else ifeq ($(TARGET),ANDROID)
OPENGL_LDLIBS = -lGLESv1_CM -ldl
else
OPENGL_LDLIBS = -lGL
endif

# Needed for native VBO support
OPENGL_CPPFLAGS += -DGL_GLEXT_PROTOTYPES

endif
