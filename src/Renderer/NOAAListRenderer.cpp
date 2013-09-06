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

#include "NOAAListRenderer.hpp"

#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Look/NOAALook.hpp"
#include "Util/StaticString.hpp"
#include "Language/Language.hpp"

namespace NOAAListRenderer
{
  static void Draw(Canvas &canvas, const PixelRect rc, PixelScalar padding_left,
                   const NOAAStore::Item &station, const DialogLook &dialog_look);
}

UPixelScalar
NOAAListRenderer::GetHeight(const DialogLook &look)
{
  return look.list.font->GetHeight() + Layout::Scale(6) +
         look.small_font->GetHeight();
}

void
NOAAListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                       PixelScalar padding_left,
                       const NOAAStore::Item &station,
                       const DialogLook &dialog_look)
{
  const unsigned padding = Layout::GetTextPadding();
  const Font &code_font = *dialog_look.list.font;
  const Font &details_font = *dialog_look.small_font;

  canvas.Select(code_font);

  StaticString<256> title;
  title = station.GetCodeT();
  if (station.parsed_metar_available &&
      station.parsed_metar.name_available)
    title.AppendFormat(_T(": %s"), station.parsed_metar.name.c_str());

  canvas.DrawClippedText(rc.left + padding + padding_left,
                         rc.top + padding, rc, title);

  canvas.Select(details_font);

  const TCHAR *tmp;
  if (!station.metar_available)
    tmp = _("No METAR available");
  else
    tmp = station.metar.content.c_str();

  canvas.DrawClippedText(rc.left + padding + padding_left,
                         rc.top + code_font.GetHeight() + Layout::FastScale(4),
                         rc, tmp);
}

void
NOAAListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                       const NOAAStore::Item &station,
                       const DialogLook &dialog_look)
{
  Draw(canvas, rc, 0, station, dialog_look);
}

void
NOAAListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                       const NOAAStore::Item &station,
                       const NOAALook &look,
                       const DialogLook &dialog_look)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  Draw(canvas, rc, line_height, station, dialog_look);

  const RasterPoint pt(rc.left + line_height / 2,
                       rc.top + line_height / 2);
  look.icon.Draw(canvas, pt);
}
