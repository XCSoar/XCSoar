/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#define USAGE "flights.log"

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/ButtonWindow.hpp"
#include "Screen/Timer.hpp"
#include "Screen/Canvas.hpp"
#include "Fonts.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "IO/FileLineReader.hpp"

enum Buttons {
  CLOSE = 1,
};

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

class MainWindow final : public SingleWindow
{
  ButtonWindow close_button;
  TestWindow test_window;

public:
  void Create(PixelSize size) {
    SingleWindow::Create(_T("Test"), size);

    WindowStyle style;
    style.Disable();

    const PixelRect rc = GetClientRect();
    close_button.Create(*this, _T("Close"), CLOSE, GetButtonRect(rc));
    test_window.Create(*this, rc, style);
  }

private:
  PixelRect GetButtonRect(PixelRect rc) {
    rc.top = rc.bottom - Layout::GetMaximumControlHeight();
    rc.left = rc.right - Layout::Scale(60);
    return rc;
  }

protected:
  virtual bool OnCommand(unsigned id, unsigned code) override {
    switch (id) {
    case CLOSE:
      Close();
      return true;
    }

    return SingleWindow::OnCommand(id, code);
  }

  virtual void OnResize(PixelSize size) override {
    SingleWindow::OnResize(size);

    const PixelRect rc = GetClientRect();
    if (test_window.IsDefined())
      test_window.Move(rc);

    if (close_button.IsDefined())
      close_button.Move(GetButtonRect(rc));
  }

  virtual bool OnKeyUp(unsigned key_code) {
    Close();
    return true;
  }

  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) {
    Close();
    return true;
  }
};

static void
ParseCommandLine(Args &args)
{
  tstring path = args.ExpectNextT();
  FileLineReaderA file(path.c_str());
  if (file.error()) {
    _ftprintf(stderr, _T("Failed to open %s\n"), path.c_str());
    return;
  }

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
