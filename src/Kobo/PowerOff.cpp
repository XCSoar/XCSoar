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

#include "Dialogs/DialogSettings.hpp"
#include "../test/src/Fonts.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/FreeType/Init.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "IO/FileLineReader.hpp"
#include "resource.h"

#include <algorithm>
#include <stdio.h>

namespace Layout {
  unsigned small_scale = 1500;
  unsigned scale_1024 = 2048;
  unsigned text_padding = 3;
};

static void
DrawBanner(Canvas &canvas, PixelRect &rc)
{
  const unsigned padding = 2;
  const Bitmap logo(IDB_LOGO);
  const unsigned banner_height = logo.GetHeight();

  int x = rc.left + padding;
  canvas.Copy(x, rc.top + padding,
              logo.GetWidth(), logo.GetHeight(),
              logo, 0, 0);

  x += logo.GetWidth() + padding;

  canvas.Select(bold_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  const TCHAR *const name = _T("XCSoar");
  canvas.Select(bold_font);
  canvas.DrawText(x, rc.top + (banner_height - bold_font.GetHeight()) / 2,
                  name);
  x += canvas.CalcTextWidth(name) + 30;

  const TCHAR *const website = _T("www.xcsoar.org");
  canvas.Select(normal_font);
  canvas.DrawText(x, rc.top + (banner_height - normal_font.GetHeight()) / 2,
                  website);

  const TCHAR *const comment = _T("powered off");
  canvas.DrawText(rc.right - canvas.CalcTextWidth(comment) - padding,
                  rc.top + padding, comment);

  rc.top += banner_height + 8;
}

static void
DrawFlights(Canvas &canvas, const PixelRect &rc)
{
  FileLineReaderA file("/mnt/onboard/XCSoarData/flights.log");
  if (file.error())
    return;

  FlightListRenderer renderer(normal_font, bold_font);

  FlightParser parser(file);
  FlightInfo flight;
  while (parser.Read(flight))
    renderer.AddFlight(flight);

  renderer.Draw(canvas, rc);
}

static void
Draw(Canvas &canvas)
{
  PixelRect rc = canvas.GetRect();
  rc.Grow(-16);

  DrawBanner(canvas, rc);
  DrawFlights(canvas, rc);
}

int main(int argc, char **argv)
{
  FreeType::Initialise();
  Font::Initialise();
  Display::Rotate(DisplaySettings::Orientation::PORTRAIT);

  InitialiseFonts();

  {
    TopCanvas screen;
    screen.Create(PixelSize{100, 100}, true, false);

    Canvas canvas = screen.Lock();
    if (canvas.IsDefined()) {
      canvas.Clear(COLOR_BLACK);
      screen.Flip();
      screen.Wait();

      screen.SetEnableDither(false);

      canvas.ClearWhite();
      Draw(canvas);
      screen.Unlock();
      screen.Flip();
      screen.Wait();
    }
  }

  execl("/sbin/poweroff", "poweroff", nullptr);
  return 0;
}
