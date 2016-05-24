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
