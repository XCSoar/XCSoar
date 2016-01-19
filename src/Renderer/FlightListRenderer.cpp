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

#include "FlightListRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Util/StaticString.hxx"

void
FlightListRenderer::AddFlight(const FlightInfo &_flight)
{
  flights.push(_flight);
}

void
FlightListRenderer::Draw(Canvas &canvas, PixelRect rc)
{
  canvas.Select(font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  if (flights.empty()) {
    auto center = rc.GetCenter();
    const TCHAR *text = _T("No flights");
    PixelSize size = canvas.CalcTextSize(text);
    canvas.DrawText(center.x - size.cx / 2, center.y - size.cy / 2, text);
    return;
  }

  const unsigned height = rc.GetHeight();

  const unsigned padding = Layout::GetTextPadding();
  const unsigned font_height = font.GetHeight();
  const unsigned header_height = header_font.GetHeight() + padding;
  if (height <= header_height)
    return;

  const unsigned row_height = font_height + padding;
  const unsigned date_width = canvas.CalcTextWidth(_T("2222-22-22")) + padding * 4;
  const unsigned time_width = canvas.CalcTextWidth(_T("22:22")) + padding * 4;

  canvas.Select(header_font);

  int y = rc.bottom - row_height * 2;
  canvas.Select(font);

  while (!flights.empty() && y > (int) rc.top + (int) header_height + (int) row_height) {
    const FlightInfo flight = flights.pop();
    int x = rc.left + padding;

    StaticString<64> buffer;
    if (flight.date.IsPlausible()) {
      buffer.UnsafeFormat(_T("%04u-%02u-%02u  "), flight.date.year,
                          flight.date.month, flight.date.day);
      canvas.DrawText(x, y, buffer);
    } else
      canvas.DrawText(x, y, _T("____-__-__"));
    x += date_width;

    if (flight.start_time.IsPlausible()) {
      buffer.UnsafeFormat(_T("%02u:%02u  "),
                          flight.start_time.hour, flight.start_time.minute);
      canvas.DrawText(x, y, buffer);
    } else
      canvas.DrawText(x, y, _T("--:--"));
    x += time_width;

    if (flight.end_time.IsPlausible()) {
      buffer.UnsafeFormat(_T("%02u:%02u"),
                          flight.end_time.hour, flight.end_time.minute);
      canvas.DrawText(x, y, buffer);
    } else
      canvas.DrawText(x, y, _T("--:--"));
    x += time_width;

    if (flight.Duration() >= 0) {
      BrokenTime duration = BrokenTime::FromSecondOfDay(flight.Duration());
      buffer.UnsafeFormat(_T("%02u:%02u"),
                          duration.hour, duration.minute);
      canvas.DrawText(x, y, buffer);
    } else
      canvas.DrawText(x, y, _T("--:--"));
    x += time_width;

    y -= row_height;
  }


  {
    int x = rc.left + padding;
    canvas.DrawText(x, y, _T("Date"));
    x += date_width;

    canvas.DrawText(x, y, _T("Time"));
    x += time_width;

    x += time_width;
    canvas.DrawText(x, y, _T("Duration"));
  }
}
