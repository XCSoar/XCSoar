EVENT_SOURCES = \
	$(SRC)/ui/event/Globals.cpp \
	$(SRC)/ui/event/Idle.cpp \
	$(SRC)/ui/event/DelayedNotify.cpp \
	$(SRC)/ui/event/Notify.cpp

ifeq ($(USE_POLL_EVENT),y)
EVENT_SOURCES += \
	$(SRC)/ui/event/poll/Timer.cpp \
	$(SRC)/ui/event/poll/Loop.cpp \
	$(SRC)/ui/event/poll/Queue.cpp
POLL_EVENT_CPPFLAGS = -DUSE_POLL_EVENT
else
EVENT_SOURCES += \
	$(SRC)/ui/event/shared/Timer.cpp \
	$(SRC)/ui/event/shared/TimerQueue.cpp
endif

ifeq ($(TARGET),ANDROID)
EVENT_SOURCES += \
	$(SRC)/ui/event/android/Loop.cpp \
	$(SRC)/ui/event/android/Queue.cpp
else ifeq ($(VFB),y)
VFB_CPPFLAGS = -DNON_INTERACTIVE
else ifeq ($(USE_X11),y)
EVENT_SOURCES += $(SRC)/ui/event/poll/X11Queue.cpp
else ifeq ($(USE_WAYLAND),y)
EVENT_SOURCES += $(SRC)/ui/event/poll/WaylandQueue.cpp
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/ui/event/poll/InputQueue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE

ifeq ($(USE_LIBINPUT),y)
EVENT_SOURCES += \
	$(SRC)/ui/event/poll/libinput/LibInputHandler.cpp
ifeq ($(ENABLE_UDEV),y)
EVENT_SOURCES += $(SRC)/ui/event/poll/libinput/UdevContext.cpp
endif
else
EVENT_SOURCES += \
	$(SRC)/ui/event/poll/linux/MergeMouse.cpp \
	$(SRC)/ui/event/poll/linux/Input.cpp
endif

else ifeq ($(ENABLE_SDL),y)
EVENT_SOURCES += \
	$(SRC)/ui/event/sdl/Loop.cpp \
	$(SRC)/ui/event/sdl/Queue.cpp
else ifeq ($(HAVE_WIN32),y)
EVENT_SOURCES += \
	$(SRC)/ui/event/windows/Loop.cpp \
	$(SRC)/ui/event/windows/Queue.cpp
endif

ifeq ($(USE_LIBINPUT),y)
$(eval $(call pkg-config-library,LIBINPUT,libinput))
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
	$(WINUSER_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) $(EGL_FEATURE_CPPFLAGS) $(GLX_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(POLL_EVENT_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

EVENT_CPPFLAGS_INTERNAL += $(EGL_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
