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

#define ENABLE_CMDLINE
#define ENABLE_SCREEN
#define ENABLE_BUTTON_LOOK
#define USAGE "flights.log"

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Timer.hpp"
#include "Screen/Canvas.hpp"
#include "Form/Button.hpp"
#include "Form/ActionListener.hpp"
#include "Fonts.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "IO/FileLineReader.hpp"

#include <vector>

static std::vector<FlightInfo> flights;

class TestWindow final : public PaintWindow {
  FlightListRenderer renderer;

public:
  TestWindow():renderer(normal_font, bold_font) {
    for (const auto &flight : flights)
      renderer.AddFlight(flight);
  }

protected:
  virtual void OnPaint(Canvas &canvas) override {
    canvas.ClearWhite();
    renderer.Draw(canvas, GetClientRect());
  }
};

class MainWindow final : public SingleWindow, ActionListener
{
  enum Buttons {
    CLOSE = 1,
  };

  Button close_button;
  TestWindow test_window;

public:
  void Create(PixelSize size) {
    SingleWindow::Create(_T("Test"), size);

    WindowStyle style;
    style.Disable();

    const PixelRect rc = GetClientRect();
    close_button.Create(*this, *button_look, _T("Close"), GetButtonRect(rc),
                        WindowStyle(),
                        *this, CLOSE);
    test_window.Create(*this, rc, style);
  }

private:
  PixelRect GetButtonRect(PixelRect rc) {
    rc.top = rc.bottom - Layout::GetMaximumControlHeight();
    rc.left = rc.right - Layout::Scale(60);
    return rc;
  }

protected:
  void OnResize(PixelSize size) override {
    SingleWindow::OnResize(size);

    const PixelRect rc = GetClientRect();
    if (test_window.IsDefined())
      test_window.Move(rc);

    if (close_button.IsDefined())
      close_button.Move(GetButtonRect(rc));
  }

  bool OnKeyUp(unsigned key_code) override {
    Close();
    return true;
  }

  bool OnMouseUp(PixelPoint p) override {
    Close();
    return true;
  }

  /* virtual methods from class ActionListener */
  void OnAction(int id) override {
    switch (id) {
    case CLOSE:
      Close();
      break;
    }
  }
};

static void
ParseCommandLine(Args &args)
{
  const auto path = args.ExpectNextPath();
  FileLineReaderA file(path);
  FlightParser parser(file);
  FlightInfo flight;
  while (parser.Read(flight))
    flights.push_back(flight);
}

static void
Main()
{
  MainWindow window;
  window.Create({500, 500});

  window.Show();
  window.RunEventLoop();
}
