/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/freetype/Init.hpp"
#include "ui/window/Init.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/FlightListRenderer.hpp"
#include "Renderer/TextRenderer.hpp"
#include "FlightInfo.hpp"
#include "Logger/FlightParser.hpp"
#include "io/FileLineReader.hpp"
#include "io/UniqueFileDescriptor.hxx"
#include "Resources.hpp"
#include "Model.hpp"
#include "Hardware/Battery.hpp"
#include "Hardware/PowerInfo.hpp"
#include "util/StringStrip.hxx"
#include "util/UTF8.hpp"

#include <algorithm>
#include <stdexcept>

#include <stdio.h>

static void
DrawBanner(Canvas &canvas, PixelRect &rc)
{
  const int padding = 2;
  const Bitmap logo(IDB_LOGO);
  const unsigned banner_height = logo.GetHeight();

  /* draw the XCSoar logo */
  int x = rc.left + padding;
  canvas.Copy({x, rc.top + padding}, logo.GetSize(),
              logo, {0, 0});

  x += logo.GetWidth() + 8;

  canvas.Select(bold_font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  /* draw the XCSoar banner text with a larger font */
  Font large_font;
  large_font.LoadFile("/opt/xcsoar/share/fonts/VeraBd.ttf", 40);
  canvas.Select(large_font);
  const int name_y = rc.top
    + (banner_height - large_font.GetHeight()) / 2;

  const TCHAR *const name1 = _T("XC");
  canvas.DrawText({x, name_y}, name1);
  x += canvas.CalcTextWidth(name1);

  const TCHAR *const name2 = _T("Soar");
  canvas.SetTextColor(COLOR_GRAY);
  canvas.DrawText({x, name_y}, name2);
  canvas.SetTextColor(COLOR_BLACK);
  x += canvas.CalcTextWidth(name2) + 30;

  /* some more text */
  const TCHAR *const website = _T("www.xcsoar.org");
  canvas.Select(normal_font);
  canvas.DrawText({x, rc.top + int(banner_height - normal_font.GetHeight()) / 2},
                  website);

  TCHAR comment[30] = _T("powered off");   
  const auto power_info = Power::GetInfo();
  if (power_info.battery.remaining_percent) {
    snprintf ( comment+strlen(comment), 30-strlen(comment), _T(" - battery %d%%"), *power_info.battery.remaining_percent);  
  }

  canvas.DrawText({rc.right - (int)canvas.CalcTextWidth(comment) - padding, rc.top + padding},
                          comment);
  rc.top += banner_height + 8;
}

/**
 * Show the contents of XCSoarData/kobo/poweroff.txt at the bottom of
 * the screen.
 */
static void
DrawUserText(Canvas &canvas, PixelRect &rc) noexcept
{
  std::array<char, 2048> buffer;

  std::string_view text;

  if (UniqueFileDescriptor fd; fd.OpenReadOnly("/mnt/onboard/XCSoarData/kobo/poweroff.txt")) {
    ssize_t nbytes = fd.Read(buffer.data(), buffer.size());
    if (nbytes <= 0)
      return;

    text = {buffer.data(), std::size_t(nbytes)};
  } else
    return;

  if (!ValidateUTF8(text))
    return;

  text = Strip(text);
  if (text.empty())
    return;

  const int padding = 2;

  canvas.Select(normal_font);

  auto text_rc = rc;
  rc.Grow(-padding);

  TextRenderer r;

  const unsigned text_height = std::min(r.GetHeight(canvas, text_rc, text),
                                        text_rc.GetHeight() / 2);
  text_rc.top = text_rc.bottom - text_height;

  const int line_y = text_rc.top - padding;

  rc.bottom = line_y - padding;

  r.Draw(canvas, text_rc, text);

  canvas.DrawHLine(text_rc.left, text_rc.right, line_y, COLOR_BLACK);
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
  DrawUserText(canvas, rc);
  DrawFlights(canvas, rc);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
  /* enable FreeType anti-aliasing, because we don't use dithering in
     this program */
  FreeType::mono = false;

  ScreenGlobalInit screen_init;
  Layout::Initialise(screen_init.GetDisplay(), {600, 800});

  Font::Initialise();
  Display::Rotate(DisplayOrientation::PORTRAIT);

  InitialiseFonts();

  {
    TopCanvas screen{screen_init.GetDisplay()};

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
