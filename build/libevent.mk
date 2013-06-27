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
else ifeq ($(EGL),y)
EVENT_SOURCES += \
	$(SRC)/Event/Shared/TimerQueue.cpp \
	$(SRC)/Event/Linux/TTYKeyboard.cpp \
	$(SRC)/Event/Linux/Mouse.cpp \
	$(SRC)/Event/EGL/Globals.cpp \
	$(SRC)/Event/EGL/Timer.cpp \
	$(SRC)/Event/EGL/Loop.cpp \
	$(SRC)/Event/EGL/Queue.cpp
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

EVENT_CPPFLAGS = $(SDL_CPPFLAGS) $(GDI_CPPFLAGS) $(OPENGL_CPPFLAGS) $(EGL_CPPFLAGS) $(MEMORY_CANVAS_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
