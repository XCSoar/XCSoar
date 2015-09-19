# use poll() based event loop
USE_POLL_EVENT ?= n

# read events from X11 (depends on USE_POLL_EVENT)
USE_X11 ?= n

# read events from console (depends on USE_POLL_EVENT)
USE_CONSOLE ?= n

# use Wayland's libinput for input device handling
ifeq ($(TARGET_IS_LINUX)$(TARGET_IS_KOBO)$(USE_CONSOLE),yny)
  # use libinput by default on Raspberry Pi and others
  USE_LIBINPUT ?= y
else
  # no libinput on the Kobo
  USE_LIBINPUT ?= n
endif

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

ifeq ($(USE_POLL_EVENT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Poll/Linux/SignalListener.cpp \
	$(SRC)/Event/Poll/Loop.cpp \
	$(SRC)/Event/Poll/Queue.cpp
POLL_EVENT_CPPFLAGS = -DUSE_POLL_EVENT
endif

ifeq ($(TARGET),ANDROID)
EVENT_SOURCES += \
	$(SRC)/Event/Android/Loop.cpp \
	$(SRC)/Event/Android/Queue.cpp
else ifeq ($(VFB),y)
VFB_CPPFLAGS = -DNON_INTERACTIVE
else ifeq ($(USE_X11),y)
EVENT_SOURCES += $(SRC)/Event/Poll/X11Queue.cpp
else ifeq ($(USE_WAYLAND),y)
EVENT_SOURCES += $(SRC)/Event/Poll/WaylandQueue.cpp
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/Event/Poll/InputQueue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE

ifeq ($(USE_LIBINPUT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Poll/LibInput/LibInputHandler.cpp
ifeq ($(ENABLE_UDEV),y)
EVENT_SOURCES += $(SRC)/Event/Poll/LibInput/UdevContext.cpp
endif
else
EVENT_SOURCES += \
	$(SRC)/Event/Poll/Linux/MergeMouse.cpp
ifeq ($(USE_LINUX_INPUT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Poll/Linux/AllInput.cpp \
	$(SRC)/Event/Poll/Linux/Input.cpp
else
EVENT_SOURCES += \
	$(SRC)/Event/Poll/Linux/TTYKeyboard.cpp \
	$(SRC)/Event/Poll/Linux/Mouse.cpp
endif
endif

else ifeq ($(ENABLE_SDL),y)
EVENT_SOURCES += \
	$(SRC)/Event/SDL/Loop.cpp \
	$(SRC)/Event/SDL/Queue.cpp
else ifeq ($(HAVE_WIN32),y)
EVENT_SOURCES += \
	$(SRC)/Event/Windows/Loop.cpp \
	$(SRC)/Event/Windows/Queue.cpp
endif

ifeq ($(USE_LINUX_INPUT),y)
LINUX_INPUT_CPPFLAGS = -DUSE_LINUX_INPUT
endif

ifeq ($(USE_LIBINPUT),y)
$(eval $(call pkg-config-library,LIBINPUT,libinput))
LIBINPUT_CPPFLAGS := $(patsubst -I%,-isystem %,$(LIBINPUT_CPPFLAGS))
EVENT_CPPFLAGS_INTERNAL += $(UDEV_CPPFLAGS)
ifeq ($(LIBINPUT_MODVERSION),0.6.0)
EVENT_CPPFLAGS_INTERNAL += -DLIBINPUT_LEGACY_API
endif
LIBINPUT_CPPFLAGS += -DUSE_LIBINPUT
EVENT_LDLIBS += $(LIBINPUT_LDLIBS) $(UDEV_LDLIBS)
endif

EVENT_CPPFLAGS = \
	$(LINUX_INPUT_CPPFLAGS) \
	$(LIBINPUT_CPPFLAGS) \
	$(SDL_CPPFLAGS) \
	$(GDI_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) $(EGL_CPPFLAGS) $(GLX_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(POLL_EVENT_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
