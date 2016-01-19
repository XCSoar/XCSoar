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

#include "NOAAListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "Look/NOAALook.hpp"
#include "Util/StaticString.hxx"
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

  const TCHAR *tmp;
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
