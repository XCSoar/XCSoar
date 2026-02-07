// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlightListRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "util/StaticString.hxx"

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
    const char *text = "No flights";
    PixelSize size = canvas.CalcTextSize(text);
    canvas.DrawText(center - size / 2u, text);
    return;
  }

  const unsigned height = rc.GetHeight();

  const unsigned padding = Layout::GetTextPadding();
  const unsigned font_height = font.GetHeight();
  const unsigned header_height = header_font.GetHeight() + padding;
  if (height <= header_height)
    return;

  const unsigned row_height = font_height + padding;
  const unsigned date_width = canvas.CalcTextWidth("2222-22-22") + padding * 4;
  const unsigned time_width = canvas.CalcTextWidth("22:22") + padding * 4;

  canvas.Select(header_font);

  int y = rc.bottom - row_height * 2;
  canvas.Select(font);

  for (auto i = flights.end();
       i-- != flights.begin() && y > int(rc.top + header_height + row_height);) {
    const auto &flight = *i;
    int x = rc.left + padding;

    StaticString<64> buffer;
    if (flight.date.IsPlausible()) {
      buffer.UnsafeFormat("%04u-%02u-%02u  ", flight.date.year,
                          flight.date.month, flight.date.day);
      canvas.DrawText({x, y}, buffer);
    } else
      canvas.DrawText({x, y}, "____-__-__");
    x += date_width;

    if (flight.start_time.IsPlausible()) {
      buffer.UnsafeFormat("%02u:%02u  ",
                          flight.start_time.hour, flight.start_time.minute);
      canvas.DrawText({x, y}, buffer);
    } else
      canvas.DrawText({x, y}, "--:--");
    x += time_width;

    if (flight.end_time.IsPlausible()) {
      buffer.UnsafeFormat("%02u:%02u",
                          flight.end_time.hour, flight.end_time.minute);
      canvas.DrawText({x, y}, buffer);
    } else
      canvas.DrawText({x, y}, "--:--");
    x += time_width;

    if (flight.Duration().count() >= 0) {
      BrokenTime duration = BrokenTime::FromSinceMidnight(flight.Duration());
      buffer.UnsafeFormat("%02u:%02u",
                          duration.hour, duration.minute);
      canvas.DrawText({x, y}, buffer);
    } else
      canvas.DrawText({x, y}, "--:--");
    x += time_width;

    y -= row_height;
  }


  {
    int x = rc.left + padding;
    canvas.DrawText({x, y}, "Date");
    x += date_width;

    canvas.DrawText({x, y}, "Time");
    x += time_width;

    x += time_width;
    canvas.DrawText({x, y}, "Duration");
  }
}
