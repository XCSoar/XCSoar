/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Traffic.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/ListPicker.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "FLARM/FlarmNet.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/FlarmId.hpp"
#include "Util/StringUtil.hpp"
#include "Language/Language.hpp"
#include "MainWindow.hpp"

#include <stdio.h>

static const FlarmId *array;

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  assert(array[index].IsDefined());

  const FlarmId id = array[index];

  const Font &name_font = Fonts::map_bold;
  const Font &small_font = Fonts::map_label;
  canvas.SetTextColor(COLOR_BLACK);

  TCHAR tmp_id[10];
  id.Format(tmp_id);

  const FlarmRecord *record = FlarmNet::FindRecordById(id);
  const TCHAR *callsign = FlarmDetails::LookupCallsign(id);

  canvas.Select(name_font);

  StaticString<256> tmp;
  if (record != NULL)
    tmp.Format(_T("%s - %s - %s"), callsign, record->registration, tmp_id);
  else if (callsign != NULL)
    tmp.Format(_T("%s - %s"), callsign, tmp_id);
  else
    tmp.Format(_T("%s"), tmp_id);

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc, tmp);

  canvas.Select(small_font);

  tmp.clear();
  if (record != NULL) {
    if (!StringIsEmpty(record->pilot))
      tmp = record->pilot;

    if (!StringIsEmpty(record->plane_type)) {
      if (!tmp.empty())
        tmp.append(_T(" - "));

      tmp.append(record->plane_type);
    }

    if (!StringIsEmpty(record->airfield)) {
      if (!tmp.empty())
        tmp.append(_T(" - "));

      tmp.append(record->airfield);
    }
  }

  if (tmp.empty())
    tmp = _("No further information");

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, tmp);
}

FlarmId
dlgFlarmDetailsListShowModal(SingleWindow &parent, const TCHAR *title,
                             FlarmId _array[], unsigned count)
{
  assert(count > 0);

  array = _array;
  UPixelScalar line_height = Fonts::map_bold.GetHeight() + Layout::Scale(6) +
                         Fonts::map_label.GetHeight();
  unsigned index = ListPicker(parent, title, count, 0, line_height, PaintListItem, true);
  return index < count
    ? array[index]
    : FlarmId::Undefined();
}
