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

#ifndef XCSOAR_EVENT_POLL_QUEUE_HPP
#define XCSOAR_EVENT_POLL_QUEUE_HPP

#include "Thread/Handle.hpp"
#include "../Shared/Event.hpp"
#include "Thread/Mutex.hpp"
#include "OS/EventPipe.hpp"
#include "IO/Async/SignalListener.hpp"

#ifdef USE_X11
#include "X11Queue.hpp"
#elif defined(USE_WAYLAND)
#include "WaylandQueue.hpp"
#elif !defined(NON_INTERACTIVE)
#include "InputQueue.hpp"
#endif

#include <boost/asio.hpp>

#include <stdint.h>

#include <queue>

enum class DisplayOrientation : uint8_t;
class Window;

/**
 * Helper class to guarantee that io_service gets initialised before
 * SignalListener.
 */
class IOServiceOwner {
protected:
  boost::asio::io_service io_service;

public:
  boost::asio::io_service &get_io_service() {
    return io_service;
  }
};

class EventQueue final : public IOServiceOwner, private SignalListener {
  const ThreadHandle thread;

#ifdef USE_X11
  X11EventQueue input_queue;
#elif defined(USE_WAYLAND)
  WaylandEventQueue input_queue;
#elif !defined(NON_INTERACTIVE)
  InputEventQueue input_queue;
#endif

  Mutex mutex;

  std::queue<Event> events;

  EventPipe event_pipe;
  boost::asio::posix::stream_descriptor event_pipe_asio;

  bool quit;

public:
  EventQueue();
  ~EventQueue();

  using IOServiceOwner::get_io_service;

#ifdef USE_X11
  _XDisplay *GetDisplay() const {
    return input_queue.GetDisplay();
  }

  bool WasCtrlClick() const {
    return input_queue.WasCtrlClick();
  }
#endif

#ifdef USE_WAYLAND
  struct wl_display *GetDisplay() {
    return input_queue.GetDisplay();
  }

  struct wl_compositor *GetCompositor() {
    return input_queue.GetCompositor();
  }

  struct wl_shell *GetShell() {
    return input_queue.GetShell();
  }
#endif

#if defined(USE_X11) || defined(USE_WAYLAND)
  bool IsVisible() const {
    return input_queue.IsVisible();
  }
#endif

  void SetScreenSize(unsigned width, unsigned height) {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
    input_queue.SetScreenSize(width, height);
#endif
  }

  void SetDisplayOrientation(DisplayOrientation orientation) {
#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND) && !defined(USE_LIBINPUT)
    input_queue.SetDisplayOrientation(orientation);
#endif
  }

#if !defined(NON_INTERACTIVE) && !defined(USE_X11) && !defined(USE_WAYLAND)
  bool HasPointer() const {
    return input_queue.HasPointer();
  }

#ifdef USE_LIBINPUT
  bool HasTouchScreen() const {
    return input_queue.HasTouchScreen();
  }

  bool HasKeyboard() const {
    return input_queue.HasKeyboard();
  }
#endif

  PixelPoint GetMousePosition() const {
    return input_queue.GetMousePosition();
  }

#endif /* !NON_INTERACTIVE */

  bool IsQuit() const {
    return quit;
  }

  void Quit() {
    quit = true;
  }

  void WakeUp() {
    if (!thread.IsInside())
      event_pipe.Signal();
  }

private:
  void Poll();
  bool Generate(Event &event);

public:
  void Push(const Event &event);

  void Push(Event::Callback callback, void *ctx) {
    Push(Event(callback, ctx));
  }

  void PushKeyPress(unsigned key_code);

  bool Pop(Event &event);

  bool Wait(Event &event);

  void Purge(bool (*match)(const Event &event, void *ctx), void *ctx);

  void Purge(enum Event::Type type);
  void Purge(Event::Callback callback, void *ctx);
  void Purge(Window &window);

private:
  /* virtual methods from SignalListener */
  void OnSignal(int signo) override;

  void AsyncReadEventPipe() {
    event_pipe_asio.async_read_some(boost::asio::null_buffers(),
                                    std::bind(&EventQueue::OnEventPipe, this,
                                              std::placeholders::_1));
  }

  void OnEventPipe(const boost::system::error_code &ec);
};

#endif
