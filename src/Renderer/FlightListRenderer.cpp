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

#include "FlightListRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Util/StaticString.hpp"

void
FlightListRenderer::AddFlight(const FlightInfo &_flight)
{
  // TODO: discard the oldest flight if the list is full
  if (flights.full())
    return;

  Flight flight;
  (FlightInfo &)flight = _flight;
  flight.duration = 3600; // TODO

  flights.push_back(flight);
}

void
FlightListRenderer::Draw(Canvas &canvas, PixelRect rc) const
{
  canvas.Select(font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  if (flights.empty()) {
    RasterPoint center = rc.GetCenter();
    const TCHAR *text = _T("No flights");
    PixelSize size = canvas.CalcTextSize(text);
    canvas.DrawText(center.x - size.cx / 2, center.y - size.cy / 2, text);
    return;
  }

  const unsigned height = rc.bottom - rc.top;

  const unsigned padding = Layout::GetTextPadding();
  const unsigned font_height = font.GetHeight();
  const unsigned header_height = header_font.GetHeight() + padding;
  if (height <= header_height)
    return;

  const unsigned row_height = font_height * 2 + padding;
  const unsigned list_height = height - header_height;
  const unsigned n_rows = std::min(list_height / row_height, flights.size());

  const unsigned date_width = canvas.CalcTextWidth(_T("2222-22-22")) + padding;
  //const unsigned time_width = canvas.CalcTextWidth(_T("22:22")) + padding;

  const unsigned date_x = rc.left + padding;
  const unsigned time_x = date_x + date_width;

  int y = rc.top;

  canvas.Select(header_font);
  canvas.DrawText(date_x, y, _T("Date"));
  canvas.DrawText(time_x, y, _T("Time"));

  y += header_height;

  canvas.Select(font);
  for (unsigned i = 0; i < n_rows; ++i, y += row_height) {
    const Flight &flight = flights[i];

    StaticString<64> buffer;
    buffer.UnsafeFormat(_T("%04u-%02u-%02u"), flight.date.year,
                        flight.date.month, flight.date.day);
    canvas.DrawText(date_x, y, buffer);

    buffer.UnsafeFormat(_T("%02u:%02u"),
                        flight.start_time.hour, flight.start_time.minute);
    canvas.DrawText(time_x, y, buffer);

    buffer.UnsafeFormat(_T("%02u:%02u"),
                        flight.end_time.hour, flight.end_time.minute);
    canvas.DrawText(time_x, y + font_height, buffer);
  }
}
