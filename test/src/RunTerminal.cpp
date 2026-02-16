// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_SCREEN

#include "Main.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/control/TerminalWindow.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "Look/TerminalLook.hpp"

class TestWindow : public UI::SingleWindow {
  TerminalWindow terminal;

  UI::PeriodicTimer timer{[this]{ WriteRandomChar(); }};

public:
  TestWindow(UI::Display &display, const TerminalLook &look)
    :UI::SingleWindow(display), terminal(look) {}

  void Create(PixelSize size) {
    SingleWindow::Create("RunTerminal", size);

    PixelRect rc = GetClientRect();

    terminal.Create(*this, rc);
  }

protected:
  void OnCreate() noexcept override {
    SingleWindow::OnCreate();
    timer.Schedule(std::chrono::milliseconds(10));
  }

  void OnDestroy() noexcept override {
    timer.Cancel();
    SingleWindow::OnDestroy();
  }

private:
  void WriteRandomChar() noexcept {
    unsigned r = rand();
    char ch;
    if ((r % 16) == 0)
      ch = '\n';
    else
      ch = 0x20 + ((r / 16) % 0x60);
    terminal.Write(&ch, 1);
  }
};

static void
Main(UI::Display &display)
{
  TerminalLook look;
  look.Initialise();

  TestWindow window{display, look};
  window.Create({400, 400});
  window.Show();

  window.RunEventLoop();
}
