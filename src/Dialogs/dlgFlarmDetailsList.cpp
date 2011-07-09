/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Screen/Graphics.hpp"
#include "FLARM/FlarmNet.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/FlarmId.hpp"
#include "Util/StringUtil.hpp"
#include "Language/Language.hpp"
#include "MainWindow.hpp"

#include <stdio.h>

const FlarmId **array;

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  assert(array[index] != NULL);
  assert(array[index]->defined());

  const FlarmId &id = *array[index];

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  TCHAR tmp[255], tmp_id[10];
  id.format(tmp_id);

  const FlarmNet::Record *record = FlarmNet::FindRecordById(id);
  const TCHAR *callsign = FlarmDetails::LookupCallsign(id);

  canvas.select(name_font);

  if (record != NULL)
    _stprintf(tmp, _T("%s - %s - %s"), callsign, record->registration, tmp_id);
  else if (callsign != NULL)
    _stprintf(tmp, _T("%s - %s"), callsign, tmp_id);
  else
    _stprintf(tmp, _T("%s"), tmp_id);

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc, tmp);

  canvas.select(small_font);

  int offset = 0;
  if (record != NULL) {
    if (!string_is_empty(record->pilot)) {
      _tcscpy(tmp, record->pilot);
      offset = _tcslen(record->pilot);
    }

    if (!string_is_empty(record->plane_type)) {
      if (offset > 0) {
        _tcscpy(tmp + offset, _T(" - "));
        offset += 3;
      }

      _tcscpy(tmp + offset, record->plane_type);
      offset += _tcslen(record->plane_type);
    }

    if (!string_is_empty(record->airfield)) {
      if (offset > 0) {
        _tcscpy(tmp + offset, _T(" - "));
        offset += 3;
      }

      _tcscpy(tmp + offset, record->airfield);
      offset = 1;
    }
  }

  if (offset == 0)
    _tcscpy(tmp, _("No further information"));

  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, tmp);
}

const FlarmId *
dlgFlarmDetailsListShowModal(SingleWindow &parent, const TCHAR *title,
                             const FlarmId *_array[], unsigned count)
{
  assert(count > 0);

  array = _array;
  unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                         Fonts::MapLabel.get_height();
  int index = ListPicker(parent, title, count, 0, line_height, PaintListItem, true);
  return (index < 0 || index >= (int)count) ? NULL : array[index];
}
