/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Queue.hpp"
#include "OS/Clock.hpp"
#include "Util/Macros.hpp"
#include "Util/CharUtil.hpp"
#include "Screen/Key.h"

#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

static struct termios restore_attr;

EventQueue::EventQueue()
  :input_state(InputState::NONE), running(true)
{
  event_pipe.Create();
  poll.SetMask(event_pipe.GetReadFD(), Poll::READ);

  /* make stdin non-blocking */
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

  struct termios attr;
  if (tcgetattr(STDIN_FILENO, &attr) == 0) {
    restore_attr = attr;
    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                      INLCR | IGNCR | ICRNL | IXON);
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG);
    attr.c_cflag &= ~(CSIZE | PARENB);
    attr.c_cflag |= CS8;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);
  }

  poll.SetMask(STDIN_FILENO, Poll::READ);
}

EventQueue::~EventQueue()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &restore_attr);
}

void
EventQueue::Push(const Event &event)
{
  ScopeLock protect(mutex);
  if (!running)
    return;

  events.push(event);
  WakeUp();
}

int
EventQueue::GetTimeout() const
{
  auto i = timers.begin();
  if (i != timers.end()) {
    int64_t relative = i->due_us - MonotonicClockUS();
    if (relative <= 0)
      return 0;
    return relative / 1000 + 1;
  }

  return -1;
}

void
EventQueue::Poll()
{
  poll.Wait(GetTimeout());
  event_pipe.Read();
}

void
EventQueue::PushKeyPress(unsigned key_code)
{
  events.push(Event(Event::KEY_DOWN, key_code));
  events.push(Event(Event::KEY_UP, key_code));
}

inline void
EventQueue::HandleInputByte(char ch)
{
  switch (ch) {
  case 0x03:
    /* user has pressed Ctrl-C */
    input_state = InputState::NONE;
    events.push(Event::CLOSE);
    return;

  case ' ':
    input_state = InputState::NONE;
    PushKeyPress(ch);
    return;

  case 0x0d:
    input_state = InputState::NONE;
    PushKeyPress(KEY_RETURN);
    return;

  case 0x1b:
    if (input_state == InputState::ESCAPE)
      PushKeyPress(KEY_ESCAPE);
    else
      input_state = InputState::ESCAPE;
    return;
  }

  switch (input_state) {
  case InputState::NONE:
    break;

  case InputState::ESCAPE:
    if (ch == '[')
      input_state = InputState::ESCAPE_BRACKET;
    else
      input_state = InputState::NONE;
    return;

  case InputState::ESCAPE_BRACKET:
    input_state = InputState::NONE;
    switch (ch) {
    case 'A':
      PushKeyPress(KEY_UP);
      break;

    case 'B':
      PushKeyPress(KEY_DOWN);
      break;

    case 'C':
      PushKeyPress(KEY_RIGHT);
      break;

    case 'D':
      PushKeyPress(KEY_LEFT);
      break;

    default:
      if (ch >= '0' && ch <= '9') {
        input_state = InputState::ESCAPE_NUMBER;
        input_number = ch - '0';
      }
    }

    return;

  case InputState::ESCAPE_NUMBER:
    if (IsDigitASCII(ch))
      input_number = input_number * 10 + ch - '0';
    else {
      input_state = InputState::NONE;
      if (ch == '~') {
        if (input_number >= 11 && input_number <= 16)
          PushKeyPress(KEY_F1 + input_number - 11);
        else if (input_number >= 17 && input_number <= 21)
          PushKeyPress(KEY_F6 + input_number - 17);
        else if (input_number >= 23 && input_number <= 24)
          PushKeyPress(KEY_F11 + input_number - 23);
      }
    }

    return;
  }

  if (IsAlphaNumericASCII(ch)) {
      PushKeyPress(ch);
      return;
  }
}

void
EventQueue::Fill()
{
  char buffer[256];
  const ssize_t nbytes = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (nbytes > 0) {
    for (ssize_t i = 0; i < nbytes; ++i)
      HandleInputByte(buffer[i]);
  } else if (nbytes == 0 || errno != EAGAIN) {
    running = false;
  }
}

bool
EventQueue::Generate(Event &event)
{
  auto t = timers.begin();
  if (t != timers.end() && t->IsDue(MonotonicClockUS())) {
    event.type = Event::TIMER;
    event.ptr = t->timer;
    timers.erase(t);
    return true;
  }

  return false;
}

bool
EventQueue::Pop(Event &event)
{
  ScopeLock protect(mutex);
  if (!running || events.empty())
    return false;

  if (events.empty()) {
    if (Generate(event))
      return true;

    Fill();
  }

  event = events.front();
  events.pop();

  if (event.type == Event::QUIT)
    Quit();

  return true;
}

bool
EventQueue::Wait(Event &event)
{
  ScopeLock protect(mutex);
  if (!running)
    return false;

  if (events.empty()) {
    if (Generate(event))
      return true;

    Fill();
    while (events.empty()) {
      mutex.Unlock();
      Poll();
      mutex.Lock();

      if (Generate(event))
        return true;

      Fill();
    }
  }

  event = events.front();
  events.pop();

  if (event.type == Event::QUIT)
    Quit();

  return true;
}

void
EventQueue::Purge(bool (*match)(const Event &event, void *ctx), void *ctx)
{
  ScopeLock protect(mutex);
  size_t n = events.size();
  while (n-- > 0) {
    if (!match(events.front(), ctx))
      events.push(events.front());
    events.pop();
  }
}

static bool
match_type(const Event &event, void *ctx)
{
  const Event::Type *type_p = (const Event::Type *)ctx;
  return event.type == *type_p;
}

void
EventQueue::Purge(Event::Type type)
{
  Purge(match_type, &type);
}

static bool
MatchCallback(const Event &event, void *ctx)
{
  const Event *match = (const Event *)ctx;
  return event.type == Event::CALLBACK && event.callback == match->callback &&
    event.ptr == match->ptr;
}

void
EventQueue::Purge(Event::Callback callback, void *ctx)
{
  Event match(callback, ctx);
  Purge(MatchCallback, (void *)&match);
}

static bool
match_window(const Event &event, void *ctx)
{
  return event.type == Event::USER && event.ptr == ctx;
}

void
EventQueue::Purge(Window &window)
{
  Purge(match_window, (void *)&window);
}

void
EventQueue::AddTimer(Timer &timer, unsigned ms)
{
  ScopeLock protect(mutex);

  timers.insert(TimerRecord(timer, MonotonicClockUS() + ms * 1000));
  WakeUp();
}

void
EventQueue::CancelTimer(Timer &timer)
{
  ScopeLock protect(mutex);

  for (auto i = timers.begin(), end = timers.end(); i != end; ++i) {
    if (i->timer == &timer) {
      timers.erase(i);
      return;
    }
  }
}
