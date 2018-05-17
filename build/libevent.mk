EVENT_SOURCES = \
	$(SRC)/Event/Globals.cpp \
	$(SRC)/Event/Idle.cpp \
	$(SRC)/Event/DelayedNotify.cpp \
	$(SRC)/Event/Notify.cpp

ifeq ($(USE_POLL_EVENT),y)
EVENT_SOURCES += \
	$(SRC)/Event/Poll/Timer.cpp \
	$(SRC)/Event/Poll/Loop.cpp \
	$(SRC)/Event/Poll/Queue.cpp
POLL_EVENT_CPPFLAGS = -DUSE_POLL_EVENT
else
EVENT_SOURCES += \
	$(SRC)/Event/Shared/Timer.cpp \
	$(SRC)/Event/Shared/TimerQueue.cpp
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
	$(SRC)/Event/Poll/Linux/MergeMouse.cpp \
	$(SRC)/Event/Poll/Linux/Input.cpp
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
