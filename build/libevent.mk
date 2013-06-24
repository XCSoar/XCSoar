USE_CONSOLE ?= $(call bool_or,$(EGL),$(USE_FB))

EVENT_SOURCES = \
	$(SRC)/Event/Idle.cpp \
	$(SRC)/Event/DelayedNotify.cpp \
	$(SRC)/Event/Notify.cpp

ifeq ($(TARGET),ANDROID)
EVENT_SOURCES += \
	$(SRC)/Event/Shared/TimerQueue.cpp \
	$(SRC)/Event/Android/Timer.cpp \
	$(SRC)/Event/Android/Loop.cpp \
	$(SRC)/Event/Android/Queue.cpp
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/Event/Shared/TimerQueue.cpp \
	$(SRC)/Event/Linux/TTYKeyboard.cpp \
	$(SRC)/Event/Linux/Mouse.cpp \
	$(SRC)/Event/Linux/Input.cpp \
	$(SRC)/Event/Console/Globals.cpp \
	$(SRC)/Event/Console/Timer.cpp \
	$(SRC)/Event/Console/Loop.cpp \
	$(SRC)/Event/Console/Queue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE
else ifeq ($(ENABLE_SDL),y)
EVENT_SOURCES += \
	$(SRC)/Event/SDL/Timer.cpp \
	$(SRC)/Event/SDL/Loop.cpp \
	$(SRC)/Event/SDL/Queue.cpp
else ifeq ($(HAVE_WIN32),y)
EVENT_SOURCES += \
	$(SRC)/Event/GDI/Transcode.cpp \
	$(SRC)/Event/GDI/Loop.cpp \
	$(SRC)/Event/GDI/Queue.cpp
endif

EVENT_CPPFLAGS = $(SDL_CPPFLAGS) $(GDI_CPPFLAGS) $(OPENGL_CPPFLAGS) $(EGL_CPPFLAGS) $(MEMORY_CANVAS_CPPFLAGS) $(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
