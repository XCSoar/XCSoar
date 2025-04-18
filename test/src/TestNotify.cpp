// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "thread/Thread.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/Globals.hpp"
#include "ui/window/Init.hpp"
#include "TestUtil.hpp"

#ifdef ANDROID
#include "ui/event/android/Loop.hpp"
#include "ui/event/shared/Event.hpp"
#include "ui/window/TopWindow.hpp"
#elif defined(USE_POLL_EVENT)
#include "ui/event/shared/Event.hpp"
#include "ui/event/poll/Loop.hpp"
#include "ui/window/TopWindow.hpp"
#elif defined(ENABLE_SDL)
#include "ui/event/sdl/Event.hpp"
#include "ui/event/sdl/Loop.hpp"
#else
#include "ui/event/windows/Event.hpp"
#include "ui/event/windows/Loop.hpp"
#endif

#ifdef USE_FB
#include "Hardware/RotateDisplay.hpp"
bool
Display::Rotate(DisplayOrientation orientation)
{
  return false;
}
#endif

#ifndef KOBO

namespace UI {

#if defined(USE_EGL) || defined(USE_GLX)
/* avoid TopWindow.cpp from being linked, as it brings some heavy
   dependencies */
void TopWindow::Refresh() noexcept {}
#endif

#ifdef USE_POLL_EVENT
void TopWindow::OnResize(PixelSize) noexcept {}
bool TopWindow::OnEvent([[maybe_unused]] const UI::Event &event) { return false; }
#endif

} // namespace UI

#endif

static bool quit;

class TestThread final : public Thread {
  UI::Notify &notify;

public:
  explicit TestThread(UI::Notify &_notify):notify(_notify) {}

protected:
  void Run() noexcept override {
    notify.SendNotification();
  }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
  plan_tests(1);

  ScreenGlobalInit screen;

  UI::EventLoop loop(*UI::event_queue);
  UI::Event event;

  UI::Notify notify{[]{ quit = true; }};
  TestThread thread(notify);
  thread.Start();

  while (!quit && loop.Get(event))
    loop.Dispatch(event);

  ok1(quit);

  thread.Join();

  return exit_status();
}
