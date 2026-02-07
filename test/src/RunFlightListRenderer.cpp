// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_CMDLINE
#define ENABLE_SCREEN
#define ENABLE_BUTTON_LOOK
#define USAGE "flights.log"

#include "Main.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Form/Button.hpp"
#include "Fonts.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "io/FileLineReader.hpp"

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
  void OnPaint(Canvas &canvas) noexcept override {
    canvas.ClearWhite();
    renderer.Draw(canvas, GetClientRect());
  }
};

class MainWindow final : public UI::SingleWindow
{
  Button close_button;
  TestWindow test_window;

public:
  using UI::SingleWindow::SingleWindow;

  void Create(PixelSize size) {
    SingleWindow::Create("Test", size);

    WindowStyle style;
    style.Disable();

    const PixelRect rc = GetClientRect();
    close_button.Create(*this, *button_look, "Close", GetButtonRect(rc),
                        WindowStyle(),
                        [this](){ Close(); });
    test_window.Create(*this, rc, style);
  }

private:
  PixelRect GetButtonRect(PixelRect rc) {
    rc.top = rc.bottom - Layout::GetMaximumControlHeight();
    rc.left = rc.right - Layout::Scale(60);
    return rc;
  }

protected:
  void OnResize(PixelSize size) noexcept override {
    SingleWindow::OnResize(size);

    const PixelRect rc = GetClientRect();
    if (test_window.IsDefined())
      test_window.Move(rc);

    if (close_button.IsDefined())
      close_button.Move(GetButtonRect(rc));
  }

  bool OnKeyUp([[maybe_unused]] unsigned key_code) noexcept override {
    Close();
    return true;
  }

  bool OnMouseUp([[maybe_unused]] PixelPoint p) noexcept override {
    Close();
    return true;
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
Main(UI::Display &display)
{
  MainWindow window{display};
  window.Create({500, 500});

  window.Show();
  window.RunEventLoop();
}
