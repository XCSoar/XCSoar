// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define ENABLE_SCREEN
#define ENABLE_CMDLINE
#define USAGE "PATH"

#include "Main.hpp"
#include "ui/window/SingleWindow.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"

class TestWindow : public UI::SingleWindow {
  Bitmap bitmap;

public:
  using UI::SingleWindow::SingleWindow;

  bool LoadFile(Path path) {
    Invalidate();
    return bitmap.LoadFile(path);
  }

protected:
  void OnPaint(Canvas &canvas) noexcept override {
    if (bitmap.IsDefined())
      canvas.Stretch(bitmap);
    else
      canvas.ClearWhite();
  }
};

static AllocatedPath path = nullptr;

static void
ParseCommandLine(Args &args)
{
  path = args.ExpectNextPath();
}

static void
Main(UI::Display &display)
{
  TestWindow window{display};
  window.Create("ViewImage", {640, 480});
  if (!window.LoadFile(path)) {
    fprintf(stderr, "Failed to load file\n");
    exit(EXIT_FAILURE);
  }

  window.RunEventLoop();
}
