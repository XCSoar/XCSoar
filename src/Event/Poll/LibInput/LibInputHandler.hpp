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

#ifndef XCSOAR_EVENT_LIBINPUT_LIBINPUT_HPP
#define XCSOAR_EVENT_LIBINPUT_LIBINPUT_HPP

#include <boost/asio/posix/stream_descriptor.hpp>

#include <assert.h>

class EventQueue;
class UdevContext;

struct libinput;
struct libinput_interface;

/**
 * A driver for handling libinput events.
 */
class LibInputHandler final {
  EventQueue &queue;

  UdevContext* udev_context = nullptr;

  struct libinput* li = nullptr;
  struct libinput_interface* li_if = nullptr;

  boost::asio::posix::stream_descriptor fd;

  double x = -1.0, y = -1.0;
  unsigned width = 0, height = 0;

  /**
   * The number of pointer input devices, touch screens ans keyboards.
   */
  unsigned n_pointers = 0, n_touch_screens = 0, n_keyboards = 0;

public:
  explicit LibInputHandler(boost::asio::io_service &io_service,
                           EventQueue &_queue)
    :queue(_queue), fd(io_service) {}

  ~LibInputHandler() {
    Close();
  }

  bool Open();
  void Close();

  void SetScreenSize(unsigned _width, unsigned _height) {
    width = _width;
    height = _height;

    assert(width > 0);
    assert(height > 0);

    if (-1.0 == x)
      x = width / 2;

    if (-1.0 == y)
      y = height / 2;
  }

  unsigned GetX() const {
    return (unsigned) x;
  }

  unsigned GetY() const {
    return (unsigned) y;
  }

  bool HasPointer() const {
    return n_pointers > 0;
  }

  bool HasTouchScreen() const {
    return n_touch_screens > 0;
  }

  bool HasKeyboard() const {
    return n_keyboards > 0;
  }

private:
  int OpenDevice(const char *path, int flags);
  void CloseDevice(int fd);

  void HandleEvent(struct libinput_event *li_event);
  void HandlePendingEvents();

  void AsyncRead() {
    fd.async_read_some(boost::asio::null_buffers(),
                       std::bind(&LibInputHandler::OnReadReady, this,
                                 std::placeholders::_1));
  }

  void OnReadReady(const boost::system::error_code &ec);
};

#endif
