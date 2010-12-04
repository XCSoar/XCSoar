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

#include "Dialogs/Dialogs.h"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Units.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "MainWindow.hpp"

static AbortTask::AlternateVector alternates;

static void
PaintListItem(Canvas &canvas, const RECT rc, unsigned index)
{
  assert(index < alternates.size());

  const Waypoint &way_point = alternates[index].first;
  const GlideResult& solution = alternates[index].second;

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  canvas.select(name_font);
  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc,
                      way_point.Name.c_str());

  TCHAR tmp[255], dist[20], alt[20];
  Units::FormatUserDistance(solution.Vector.Distance, dist, 20, true);
  Units::FormatUserArrival(solution.AltitudeDifference, alt, 20, true);
  _stprintf(tmp, _T("Distance: %s - Arrival Altitude: %s"), dist, alt);

  canvas.select(small_font);
  canvas.text_clipped(rc.left + Layout::FastScale(2),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, tmp);
}

void
dlgAlternatesListShowModal(SingleWindow &parent)
{
  alternates = protected_task_manager.getAlternates();
  int i = ListPicker(parent, _("Best landing options"), alternates.size(), 0,
                     Layout::Scale(30), PaintListItem, NULL);

  if (i < 0 || (unsigned)i >= alternates.size())
    return;

  dlgWayPointDetailsShowModal(parent, alternates[i].first);
}
