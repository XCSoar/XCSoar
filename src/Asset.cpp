// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Asset.hpp"
#include <fcntl.h>

#if defined(USE_CONSOLE) || defined(USE_WAYLAND)
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#if (defined(USE_CONSOLE) && !defined(KOBO)) || defined(USE_WAYLAND)

bool
HasPointer() noexcept
{
  return UI::event_queue->HasPointer();
}

#endif

#if defined(USE_LIBINPUT) || defined(USE_WAYLAND)

#define MAX_EVENT_DEVICES 20
#define EVENT_SYSFS_PREFIX "/sys/class/input/event"
#define EVENT_SYSFS_POSTFIX "device/name"
#define MAX_FNAME 40
#define MAX_DNAME 80
#define TS_STRING "TouchScreen"

bool
HasTouchScreen() noexcept
{
  static enum {
    not_detected,
    not_attached,
    attached
  } touchscreen = not_detected;

  if (touchscreen != not_detected)
    return touchscreen == attached;
  if (UI::event_queue->HasTouchScreen()) {
    touchscreen = attached;
    return true;
  }
  touchscreen = not_attached;
  for (int i = 0; i <  MAX_EVENT_DEVICES; i++) {
    char fname[MAX_FNAME], dname[MAX_DNAME];
    int eventf;
    ssize_t count;

    snprintf(fname, sizeof(fname),
	     "%s%d/%s",
	     EVENT_SYSFS_PREFIX, i, EVENT_SYSFS_POSTFIX);
    eventf = open(fname, O_RDONLY);
    if (eventf < 0)
      break;
    count = read(eventf, dname, sizeof(dname));
    if (count <= 0) {
      close(eventf);
      break;
    }
    if (count >= (ssize_t)sizeof(dname))
      count = sizeof(dname) - 1;
    dname[count] = '\0';
    if (strstr(dname, TS_STRING)) {
      touchscreen = attached;
      close(eventf);
      return true;
    }
    close(eventf);
  }
  return false;
}

bool
HasKeyboard() noexcept
{
  return UI::event_queue->HasKeyboard();
}

#endif
