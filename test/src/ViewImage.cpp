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

#define ENABLE_SCREEN
#define ENABLE_CMDLINE
#define USAGE "PATH"

#include "Main.hpp"
#include "Screen/SingleWindow.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"

class TestWindow : public SingleWindow {
  Bitmap bitmap;

public:
  bool LoadFile(Path path) {
    Invalidate();
    return bitmap.LoadFile(path);
  }

protected:
  virtual void OnPaint(Canvas &canvas) override {
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
Main()
{
  TestWindow window;
  window.Create(_T("ViewImage"), {640, 480});
  if (!window.LoadFile(path)) {
    fprintf(stderr, "Failed to load file\n");
    exit(EXIT_FAILURE);
  }

  window.RunEventLoop();
}
