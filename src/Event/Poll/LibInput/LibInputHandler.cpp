/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "LibInputHandler.hpp"
#include "UdevContext.hpp"
#include "Event/Queue.hpp"
#include "Event/Shared/Event.hpp"
#include "Event/Poll/Linux/Translate.hpp"
#include "Util/Clamp.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include <libinput.h>

bool
LibInputHandler::Open()
{
  if ((nullptr != udev_context)
      || (nullptr != li_if)
      || (nullptr != li)
      || fd.is_open())
    return false;

  if (nullptr == udev_context) {
    udev_context = new UdevContext(UdevContext::NewRef());
    if ((nullptr == udev_context) || (nullptr == udev_context->Get())) {
      return false;
    }
  }

  li_if = new libinput_interface;
  assert(li_if);
  li_if->open_restricted = [](const char *path, int flags, void* user_data)
      -> int {
    return reinterpret_cast<LibInputHandler*>(user_data)->OpenDevice(path,
                                                                     flags);
  };
  li_if->close_restricted = [](int fd, void* user_data) {
    reinterpret_cast<LibInputHandler*>(user_data)->CloseDevice(fd);
  };

  li = libinput_udev_create_context(li_if, this, udev_context->Get());
  if (nullptr == li)
    return false;

  int assign_seat_ret = libinput_udev_assign_seat(li, UDEV_DEFAULT_SEAT);
  if (0 != assign_seat_ret)
    return false;

  int _fd = libinput_get_fd(li);
  if (_fd < 0)
    return false;

  fd.assign(_fd);
  AsyncRead();
  return true;
}

void
LibInputHandler::Close()
{
  if (fd.is_open()) {
    fd.cancel();
    fd.release();
  }

  if (nullptr != li)
    libinput_unref(li);
  li = nullptr;

  if (nullptr != li_if)
    delete li_if;
  li_if = nullptr;

  delete udev_context;
  udev_context = nullptr;
}

int
LibInputHandler::OpenDevice(const char *path, int flags)
{
  int fd = open(path, flags);
  if (fd < 0)
    return -errno;

  return fd;
}

void
LibInputHandler::CloseDevice(int fd)
{
  close(fd);
}

inline void
LibInputHandler::HandleEvent(struct libinput_event *li_event)
{
  int type = libinput_event_get_type(li_event);
  switch (type) {
  case LIBINPUT_EVENT_DEVICE_ADDED:
  case LIBINPUT_EVENT_DEVICE_REMOVED:
    {
      libinput_device *event_device = libinput_event_get_device(li_event);
      assert(nullptr != event_device);
      if (libinput_device_has_capability(event_device,
                                         LIBINPUT_DEVICE_CAP_POINTER)) {
        n_pointers += (LIBINPUT_EVENT_DEVICE_ADDED == type) ? 1 : -1;
      }
      if (libinput_device_has_capability(event_device,
                                         LIBINPUT_DEVICE_CAP_TOUCH)) {
        n_touch_screens += (LIBINPUT_EVENT_DEVICE_ADDED == type) ? 1 : -1;
      }
      if (libinput_device_has_capability(event_device,
                                         LIBINPUT_DEVICE_CAP_KEYBOARD)) {
        n_keyboards += (LIBINPUT_EVENT_DEVICE_ADDED == type) ? 1 : -1;
      }
    }
    break;

  case LIBINPUT_EVENT_KEYBOARD_KEY:
    {
      /* Discard all data on stdin to avoid that keyboard input data is read
       * on the executing shell. */
      tcflush(STDIN_FILENO, TCIFLUSH);

      libinput_event_keyboard *kb_li_event =
        libinput_event_get_keyboard_event(li_event);
      uint32_t key_code = libinput_event_keyboard_get_key(kb_li_event);
      libinput_key_state key_state =
        libinput_event_keyboard_get_key_state(kb_li_event);
      bool is_char;
      Event e(key_state == LIBINPUT_KEY_STATE_PRESSED
                  ? Event::KEY_DOWN : Event::KEY_UP,
              TranslateKeyCode(key_code, is_char));
      e.is_char = is_char;
      queue.Push(e);
    }
    break;
  case LIBINPUT_EVENT_POINTER_MOTION:
    {
      libinput_event_pointer *ptr_li_event =
        libinput_event_get_pointer_event(li_event);
      if (-1.0 == x)
        x = 0.0;
      if (-1.0 == y)
        y = 0.0;
      x += libinput_event_pointer_get_dx(ptr_li_event);
      x = Clamp<double>(x, 0, width);
      y += libinput_event_pointer_get_dy(ptr_li_event);
      y = Clamp<double>(y, 0, height);
      queue.Push(Event(Event::MOUSE_MOTION,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
    {
      libinput_event_pointer *ptr_li_event =
        libinput_event_get_pointer_event(li_event);
      x = libinput_event_pointer_get_absolute_x_transformed(ptr_li_event,
                                                            width);
      y = libinput_event_pointer_get_absolute_y_transformed(ptr_li_event,
                                                            height);
      queue.Push(Event(Event::MOUSE_MOTION,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  case LIBINPUT_EVENT_POINTER_BUTTON:
    {
      libinput_event_pointer *ptr_li_event =
        libinput_event_get_pointer_event(li_event);
      libinput_button_state btn_state =
        libinput_event_pointer_get_button_state(ptr_li_event);
      queue.Push(Event(btn_state == LIBINPUT_BUTTON_STATE_PRESSED
                       ? Event::MOUSE_DOWN
                       : Event::MOUSE_UP,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  case LIBINPUT_EVENT_POINTER_AXIS:
    {
      libinput_event_pointer *ptr_li_event =
        libinput_event_get_pointer_event(li_event);
#ifdef LIBINPUT_LEGACY_API
      double axis_value = libinput_event_pointer_get_axis_value(ptr_li_event);
#else
      double axis_value =
        libinput_event_pointer_get_axis_value(
          ptr_li_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
#endif
      if (0 != axis_value) {
        Event event(Event::MOUSE_WHEEL,
                    PixelPoint((unsigned)x, (unsigned)y));
        event.param = unsigned((int) axis_value);
        queue.Push(event);
      }
    }
    break;
  case LIBINPUT_EVENT_TOUCH_DOWN:
    {
      libinput_event_touch *touch_li_event =
        libinput_event_get_touch_event(li_event);
      x = libinput_event_touch_get_x_transformed(touch_li_event, width);
      y = libinput_event_touch_get_y_transformed(touch_li_event, height);
      queue.Push(Event(Event::MOUSE_DOWN,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  case LIBINPUT_EVENT_TOUCH_UP:
    {
      queue.Push(Event(Event::MOUSE_UP,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  case LIBINPUT_EVENT_TOUCH_MOTION:
    {
      libinput_event_touch *touch_li_event =
        libinput_event_get_touch_event(li_event);
      x = libinput_event_touch_get_x_transformed(touch_li_event, width);
      y = libinput_event_touch_get_y_transformed(touch_li_event, height);
      queue.Push(Event(Event::MOUSE_MOTION,
                       PixelPoint((unsigned)x, (unsigned)y)));
    }
    break;
  }
}

inline void
LibInputHandler::HandlePendingEvents()
{
  libinput_dispatch(li);
  for (libinput_event *li_event = libinput_get_event(li);
       nullptr != li_event;
       li_event = libinput_get_event(li))
    HandleEvent(li_event);
}

void
LibInputHandler::OnReadReady(const boost::system::error_code &ec)
{
  if (ec)
    return;

  HandlePendingEvents();
  AsyncRead();
}
