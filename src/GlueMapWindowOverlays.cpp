/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "GlueMapWindow.hpp"

#include "Screen/Icon.hpp"
#include "Language.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Logger/Logger.hpp"
#include "Task/ProtectedTaskManager.hpp"

void
GlueMapWindow::DrawCrossHairs(Canvas &canvas) const
{
  Pen dash_pen(Pen::DASH, 1, Color(50, 50, 50));
  canvas.select(dash_pen);

  const POINT Orig_Screen = render_projection.GetScreenOrigin();

  canvas.line(Orig_Screen.x + 20, Orig_Screen.y,
              Orig_Screen.x - 20, Orig_Screen.y);
  canvas.line(Orig_Screen.x, Orig_Screen.y + 20,
              Orig_Screen.x, Orig_Screen.y - 20);
}

void
GlueMapWindow::DrawGPSStatus(Canvas &canvas, const RECT &rc,
                             const GPS_STATE &gps) const
{
  const TCHAR *txt;
  MaskedIcon *icon = NULL;

  if (!gps.Connected) {
    icon = &Graphics::hGPSStatus2;
    txt = _("GPS not connected");
  } else if (gps.NAVWarning || (gps.SatellitesUsed == 0)) {
    icon = &Graphics::hGPSStatus1;
    txt = _("GPS waiting for fix");
  } else {
    return; // early exit
  }

  icon->draw(canvas, bitmap_canvas,
             rc.left + IBLSCALE(2),
            rc.bottom + IBLSCALE(-35));

  canvas.background_opaque();
  canvas.set_background_color(Color::WHITE);
  canvas.set_text_color(Color::BLACK);
  canvas.text(rc.left + IBLSCALE(24), rc.bottom + IBLSCALE(-32),
              txt);
}

void
GlueMapWindow::DrawFlightMode(Canvas &canvas, const RECT &rc) const
{
  static bool flip = true;
  static fixed LastTime = fixed_zero;
  bool drawlogger = true;
  static bool lastLoggerActive = false;
  int offset = -1;

  // has GPS time advanced?
  if (Basic().Time <= LastTime) {
    LastTime = Basic().Time;
  } else {
    flip = !flip;

    // don't bother drawing logger if not active for more than one second
    if ((!logger.isLoggerActive()) && (!lastLoggerActive))
      drawlogger = false;

    lastLoggerActive = logger.isLoggerActive();
  }

  if (drawlogger) {
    offset -= 7;
    MaskedIcon &icon = (logger.isLoggerActive() && flip) ?
                       Graphics::hLogger : Graphics::hLoggerOff;

    icon.draw(canvas, bitmap_canvas,
              rc.right + IBLSCALE(offset),
              rc.bottom + IBLSCALE(-7));
  }

  MaskedIcon *bmp;

  if (task != NULL && (task->get_mode() == TaskManager::MODE_ABORT))
    bmp = &Graphics::hAbort;
  else if (render_projection.GetDisplayMode() == dmCircling)
    bmp = &Graphics::hClimb;
  else if (render_projection.GetDisplayMode() == dmFinalGlide)
    bmp = &Graphics::hFinalGlide;
  else
    bmp = &Graphics::hCruise;

  offset -= 24;

  bmp->draw(canvas, bitmap_canvas, rc.right + IBLSCALE(offset - 1),
            rc.bottom + IBLSCALE(-21));
}
