/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
    SingleWindow::Create(_T("RunTerminal"), size);

    PixelRect rc = GetClientRect();

    terminal.Create(*this, rc);
  }

protected:
  virtual void OnCreate() override {
    SingleWindow::OnCreate();
    timer.Schedule(std::chrono::milliseconds(10));
  }

  virtual void OnDestroy() override {
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
