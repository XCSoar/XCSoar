USE_CONSOLE ?= $(call bool_or,$(EGL),$(USE_FB))

# use Wayland's libinput for input device handling
USE_LIBINPUT ?= n

ifeq ($(USE_LIBINPUT),n)
# query /dev/input/event* instead of stdin and /dev/input/mice or libinput?
USE_LINUX_INPUT ?= $(call bool_and,$(TARGET_IS_LINUX),$(USE_CONSOLE))
else
USE_LINUX_INPUT = n
endif

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
	$(SRC)/Event/Console/Loop.cpp \
	$(SRC)/Event/Console/Queue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE

ifeq ($(USE_LIBINPUT),y)
EVENT_SOURCES += \
	$(SRC)/Event/LibInput/LibInputHandler.cpp
ifeq ($(ENABLE_UDEV),y)
EVENT_SOURCES += $(SRC)/Event/LibInput/UdevContext.cpp
endif
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/Event/Linux/MergeMouse.cpp
ifeq ($(USE_LINUX_INPUT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Linux/AllInput.cpp \
	$(SRC)/Event/Linux/Input.cpp
else
EVENT_SOURCES += \
	$(SRC)/Event/Linux/TTYKeyboard.cpp \
	$(SRC)/Event/Linux/Mouse.cpp
endif
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

ifeq ($(USE_LIBINPUT),y)
$(eval $(call pkg-config-library,LIBINPUT,libinput))
LIBINPUT_CPPFLAGS := $(patsubst -I%,-isystem %,$(LIBINPUT_CPPFLAGS))
LIBINPUT_CPPFLAGS_INTERNAL += $(UDEV_CPPFLAGS)
LIBINPUT_CPPFLAGS += -DUSE_LIBINPUT
EVENT_LDLIBS += $(LIBINPUT_LDLIBS) $(UDEV_LDLIBS)
endif

EVENT_CPPFLAGS = \
	$(LINUX_INPUT_CPPFLAGS) \
	$(LIBINPUT_CPPFLAGS) \
	$(SDL_CPPFLAGS) \
	$(GDI_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) $(EGL_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
