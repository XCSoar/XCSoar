// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOAAListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "Look/NOAALook.hpp"
#include "ui/dim/Rect.hpp"
#include "util/StaticString.hxx"
#include "Language/Language.hpp"

void
NOAAListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                       const NOAAStore::Item &station,
                       const TwoTextRowsRenderer &row_renderer)
{
  StaticString<256> title;
  title = station.GetCodeT();
  if (station.parsed_metar_available &&
      station.parsed_metar.name_available)
    title.AppendFormat(_T(": %s"), station.parsed_metar.name.c_str());

  row_renderer.DrawFirstRow(canvas, rc, title);

  const char *tmp;
  if (!station.metar_available)
    tmp = _("No METAR available");
  else
    tmp = station.metar.content.c_str();

  row_renderer.DrawSecondRow(canvas, rc, tmp);
}

void
NOAAListRenderer::Draw(Canvas &canvas, PixelRect rc,
                       const NOAAStore::Item &station,
                       const NOAALook &look,
                       const TwoTextRowsRenderer &row_renderer)
{
  const unsigned line_height = rc.GetHeight();

  const PixelPoint pt(rc.left + line_height / 2,
                      rc.top + line_height / 2);
  look.icon.Draw(canvas, pt);

  rc.left += line_height;

  Draw(canvas, rc, station, row_renderer);
}
