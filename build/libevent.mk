USE_CONSOLE ?= $(call bool_or,$(EGL),$(USE_FB))

# query /dev/input/event* instead of stdin and /dev/input/mice?
USE_LINUX_INPUT ?= $(call bool_and,$(TARGET_IS_LINUX),$(USE_CONSOLE))

EVENT_SOURCES = \
	$(SRC)/Event/Shared/Timer.cpp \
	$(SRC)/Event/Shared/TimerQueue.cpp \
	$(SRC)/Event/Globals.cpp \
	$(SRC)/Event/Idle.cpp \
	$(SRC)/Event/DelayedNotify.cpp \
	$(SRC)/Event/Notify.cpp

ifeq ($(TARGET),ANDROID)
EVENT_SOURCES += \
	$(SRC)/Event/Android/Loop.cpp \
	$(SRC)/Event/Android/Queue.cpp
else ifeq ($(VFB),y)
EVENT_SOURCES += \
	$(SRC)/Event/Linux/SignalListener.cpp \
	$(SRC)/Event/Console/Loop.cpp \
	$(SRC)/Event/Console/Queue.cpp
VFB_CPPFLAGS = -DNON_INTERACTIVE
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/Event/Linux/SignalListener.cpp \
	$(SRC)/Event/Linux/MergeMouse.cpp \
	$(SRC)/Event/Console/Loop.cpp \
	$(SRC)/Event/Console/Queue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE

ifeq ($(USE_LINUX_INPUT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Linux/AllInput.cpp \
	$(SRC)/Event/Linux/Input.cpp
else
EVENT_SOURCES += \
	$(SRC)/Event/Linux/TTYKeyboard.cpp \
	$(SRC)/Event/Linux/Mouse.cpp
endif

else ifeq ($(ENABLE_SDL),y)
EVENT_SOURCES += \
	$(SRC)/Event/SDL/Loop.cpp \
	$(SRC)/Event/SDL/Queue.cpp
else ifeq ($(HAVE_WIN32),y)
EVENT_SOURCES += \
	$(SRC)/Event/GDI/Transcode.cpp \
	$(SRC)/Event/GDI/Loop.cpp \
	$(SRC)/Event/GDI/Queue.cpp
endif

ifeq ($(USE_LINUX_INPUT),y)
LINUX_INPUT_CPPFLAGS = -DUSE_LINUX_INPUT
endif

EVENT_CPPFLAGS = \
	$(LINUX_INPUT_CPPFLAGS) \
	$(SDL_CPPFLAGS) \
	$(GDI_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) $(EGL_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
