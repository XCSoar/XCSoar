EVENT_SOURCES = \
	$(SRC)/event/Globals.cpp \
	$(SRC)/event/Idle.cpp \
	$(SRC)/event/DelayedNotify.cpp \
	$(SRC)/event/Notify.cpp

ifeq ($(USE_POLL_EVENT),y)
EVENT_SOURCES += \
	$(SRC)/event/poll/Timer.cpp \
	$(SRC)/event/poll/Loop.cpp \
	$(SRC)/event/poll/Queue.cpp
POLL_EVENT_CPPFLAGS = -DUSE_POLL_EVENT
else
EVENT_SOURCES += \
	$(SRC)/event/shared/Timer.cpp \
	$(SRC)/event/shared/TimerQueue.cpp
endif

ifeq ($(TARGET),ANDROID)
EVENT_SOURCES += \
	$(SRC)/event/android/Loop.cpp \
	$(SRC)/event/android/Queue.cpp
else ifeq ($(VFB),y)
VFB_CPPFLAGS = -DNON_INTERACTIVE
else ifeq ($(USE_X11),y)
EVENT_SOURCES += $(SRC)/event/poll/X11Queue.cpp
else ifeq ($(USE_WAYLAND),y)
EVENT_SOURCES += $(SRC)/event/poll/WaylandQueue.cpp
else ifeq ($(USE_CONSOLE),y)
EVENT_SOURCES += \
	$(SRC)/event/poll/InputQueue.cpp
CONSOLE_CPPFLAGS = -DUSE_CONSOLE

ifeq ($(USE_LIBINPUT),y)
EVENT_SOURCES += \
	$(SRC)/event/poll/libinput/LibInputHandler.cpp
ifeq ($(ENABLE_UDEV),y)
EVENT_SOURCES += $(SRC)/event/poll/libinput/UdevContext.cpp
endif
else
EVENT_SOURCES += \
	$(SRC)/event/poll/linux/MergeMouse.cpp \
	$(SRC)/event/poll/linux/Input.cpp
endif

else ifeq ($(ENABLE_SDL),y)
EVENT_SOURCES += \
	$(SRC)/event/sdl/Loop.cpp \
	$(SRC)/event/sdl/Queue.cpp
else ifeq ($(HAVE_WIN32),y)
EVENT_SOURCES += \
	$(SRC)/event/windows/Loop.cpp \
	$(SRC)/event/windows/Queue.cpp
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
	$(GDI_CPPFLAGS) \
	$(OPENGL_CPPFLAGS) $(EGL_FEATURE_CPPFLAGS) $(GLX_CPPFLAGS) \
	$(MEMORY_CANVAS_CPPFLAGS) \
	$(POLL_EVENT_CPPFLAGS) \
	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)

EVENT_CPPFLAGS_INTERNAL += $(EGL_CPPFLAGS)

$(eval $(call link-library,libevent,EVENT))
