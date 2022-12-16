set(EVENT_SRC_DIR    ui/event)
#######################################################################

set(EVENT_SOURCES
  ${EVENT_SRC_DIR}/Globals.cpp
  ${EVENT_SRC_DIR}/Idle.cpp
  ${EVENT_SRC_DIR}/DelayedNotify.cpp
  ${EVENT_SRC_DIR}/Notify.cpp
)

if(USE_POLL_EVENT)
  list(APPEND EVENT_SOURCES
    ${EVENT_SRC_DIR}/poll/Timer.cpp
    ${EVENT_SRC_DIR}/poll/Loop.cpp
    ${EVENT_SRC_DIR}/poll/Queue.cpp
####          ${EVENT_SRC_DIR}/poll/X11Queue.cpp
  )
else()
  list(APPEND EVENT_SOURCES
          ${EVENT_SRC_DIR}/shared/Timer.cpp
          ${EVENT_SRC_DIR}/shared/TimerQueue.cpp
  )
endif()	  
	  
if(ANDROID)   # TARGET_IS_ANDROID
  list(APPEND EVENT_SOURCES
    ${EVENT_SRC_DIR}/android/Loop.cpp
    ${EVENT_SRC_DIR}/android/Queue.cpp
  )
elseif(VFB)
  set(VFB_CPPFLAGS "-DNON_INTERACTIVE")
elseif(USE_X11)
  list(APPEND EVENT_SOURCES 
    ${EVENT_SRC_DIR}/poll/X11Queue.cpp
  )
elseif(USE_WAYLAND)
  list(APPEND EVENT_SOURCES 
    ${EVENT_SRC_DIR}/poll/WaylandQueue.cpp
  )
elseif(USE_CONSOLE)
  list(APPEND EVENT_SOURCES
    ${EVENT_SRC_DIR}/poll/InputQueue.cpp)
  set(CONSOLE_CPPFLAGS "-DUSE_CONSOLE")

  if(USE_LIBINPUT)
    list(APPEND EVENT_SOURCES
      ${EVENT_SRC_DIR}/poll/libinput/LibInputHandler.cpp
    )
    if(ENABLE_UDEV)
        list(APPEND EVENT_SOURCES
          ${EVENT_SRC_DIR}/poll/libinput/UdevContext.cpp
        )
    endif()
  else()
    list(APPEND EVENT_SOURCES
      ${EVENT_SRC_DIR}/poll/linux/MergeMouse.cpp
      ${EVENT_SRC_DIR}/poll/linux/Input.cpp
    )
  endif()
elseif(ENABLE_SDL)
        list(APPEND EVENT_SOURCES
            ${EVENT_SRC_DIR}/sdl/Loop.cpp
            ${EVENT_SRC_DIR}/sdl/Queue.cpp
        )
elseif(WIN32)  # TARGET_IS_WINDOWS
        list(APPEND EVENT_SOURCES
            ${EVENT_SRC_DIR}/windows/Loop.cpp
            ${EVENT_SRC_DIR}/windows/Queue.cpp
        )
endif()

### ifeq ($(USE_LIBINPUT),y)
### $(eval $(call pkg-config-library,LIBINPUT,libinput))
### EVENT_CPPFLAGS_INTERNAL += $(UDEV_CPPFLAGS)
### ifeq ($(LIBINPUT_MODVERSION),0.6.0)
### EVENT_CPPFLAGS_INTERNAL += -DLIBINPUT_LEGACY_API
### endif
### LIBINPUT_CPPFLAGS += -DUSE_LIBINPUT
### EVENT_LDLIBS += $(LIBINPUT_LDLIBS) $(UDEV_LDLIBS)
### endif
### 
### EVENT_CPPFLAGS = \
### 	$(LINUX_INPUT_CPPFLAGS) \
### 	$(LIBINPUT_CPPFLAGS) \
### 	$(SDL_CPPFLAGS) \
### 	$(WINUSER_CPPFLAGS) \
### 	$(OPENGL_CPPFLAGS) $(EGL_FEATURE_CPPFLAGS) $(GLX_CPPFLAGS) \
### 	$(MEMORY_CANVAS_CPPFLAGS) \
### 	$(POLL_EVENT_CPPFLAGS) \
### 	$(CONSOLE_CPPFLAGS) $(FB_CPPFLAGS) $(VFB_CPPFLAGS)
### 
### EVENT_CPPFLAGS_INTERNAL += $(EGL_CPPFLAGS)
### 
### $(eval $(call link-library,libevent,EVENT))
