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

#include "Dialogs/DialogSettings.hpp"
#include "../test/src/Fonts.hpp"
#include "Hardware/RotateDisplay.hpp"
#include "DisplayOrientation.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Custom/TopCanvas.hpp"
#include "Screen/FreeType/Init.hpp"
#include "Screen/Init.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "IO/FileLineReader.hpp"
#include "Resources.hpp"
#include "Model.hpp"

#include <algorithm>
#include <stdexcept>

#include <stdio.h>

static void
DrawBanner(Canvas &canvas, PixelRect &rc)
{
  const unsigned padding = 2;
  const Bitmap logo(IDB_LOGO);
  const unsigned banner_height = logo.GetHeight();

  /* draw the XCSoar logo */
  int x = rc.left + padding;
  canvas.Copy(x, rc.top + padding,
              logo.GetWidth(), logo.GetHeight(),
              logo, 0, 0);

  x += logo.GetWidth() + 8;

  canvas.Select(bold_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  /* draw the XCSoar banner text with a larger font */
  Font large_font;
  large_font.LoadFile("/opt/xcsoar/share/fonts/VeraBd.ttf", 40);
  canvas.Select(large_font);
  const unsigned name_y = rc.top
    + (banner_height - large_font.GetHeight()) / 2;

  const TCHAR *const name1 = _T("XC");
  canvas.DrawText(x, name_y, name1);
  x += canvas.CalcTextWidth(name1);

  const TCHAR *const name2 = _T("Soar");
  canvas.SetTextColor(COLOR_GRAY);
  canvas.DrawText(x, name_y, name2);
  canvas.SetTextColor(COLOR_BLACK);
  x += canvas.CalcTextWidth(name2) + 30;

  /* some more text */
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
try {
  FileLineReaderA file(Path("/mnt/onboard/XCSoarData/flights.log"));

  FlightListRenderer renderer(normal_font, bold_font);

  FlightParser parser(file);
  FlightInfo flight;
  while (parser.Read(flight))
    renderer.AddFlight(flight);

  renderer.Draw(canvas, rc);
} catch (const std::runtime_error &e) {
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
  /* enable FreeType anti-aliasing, because we don't use dithering in
     this program */
  FreeType::mono = false;

  ScreenGlobalInit screen_init;
  Layout::Initialize({600, 800});

  Font::Initialise();
  Display::Rotate(DisplayOrientation::PORTRAIT);

  InitialiseFonts();

  {
    TopCanvas screen;
    screen.Create(PixelSize(100, 100), true, false);

    Canvas canvas = screen.Lock();
    if (canvas.IsDefined()) {
      /* all black first, to eliminate E-ink ghost images */
      canvas.Clear(COLOR_BLACK);
      screen.Flip();
      screen.Wait();

      /* disable dithering, render with 16 shades of gray, to make the
         (static) display more pretty */
      screen.SetEnableDither(false);

      /* draw the pictuer */
      canvas.ClearWhite();
      Draw(canvas);

      /* finish */
      screen.Unlock();
      screen.Flip();
      screen.Wait();
    }
  }

  /* now we can power off the Kobo; the picture remains on the
     screen */
  if (DetectKoboModel() == KoboModel::GLO_HD)
    //The GloHD needs -f to not clear screen
    execl("/sbin/poweroff", "poweroff", "-f", nullptr);
  else
    execl("/sbin/poweroff", "poweroff", nullptr);

  return 0;
}
