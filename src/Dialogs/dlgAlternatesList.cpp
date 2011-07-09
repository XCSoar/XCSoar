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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Waypoint.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Icon.hpp"
#include "Units/UnitsFormatter.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "MainWindow.hpp"
#include "WaypointRenderer.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

static AbortTask::AlternateVector alternates;

static void
UpdateAlternates()
{
  ProtectedTaskManager::Lease lease(*protected_task_manager);
  alternates = lease->getAlternates();
}

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned index)
{
  assert(index < alternates.size());

  const unsigned line_height = rc.bottom - rc.top;

  const Waypoint &way_point = alternates[index].waypoint;
  const GlideResult& solution = alternates[index].solution;

  // Draw icon
  RasterPoint pt = { rc.left + line_height / 2,
                     rc.top + line_height / 2};

  WaypointRenderer::DrawLandableSymbol(canvas, pt,
                                       positive(solution.AltitudeDifference),
                                       true, way_point);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  // Draw waypoint name
  canvas.select(name_font);
  canvas.text_clipped(rc.left + line_height + Layout::FastScale(2),
                      rc.top + Layout::FastScale(2), rc,
                      way_point.Name.c_str());

  // Draw distance and arrival altitude
  TCHAR tmp[255], dist[20], alt[20];
  Units::FormatUserDistance(solution.Vector.Distance, dist, 20, true);
  Units::FormatUserArrival(solution.AltitudeDifference, alt, 20, true);
  _stprintf(tmp, _T("Distance: %s - Arrival Alt: %s"), dist, alt);

  canvas.select(small_font);
  canvas.text_clipped(rc.left + line_height + Layout::FastScale(2),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, tmp);
}

void
dlgAlternatesListShowModal(SingleWindow &parent)
{
  if (protected_task_manager == NULL)
    return;

  UpdateAlternates();
  unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                         Fonts::MapLabel.get_height();
  int i = ListPicker(parent, _("Alternates"), alternates.size(), 0,
                     line_height, PaintListItem, true);

  if (i < 0 || (unsigned)i >= alternates.size())
    return;

  dlgWaypointDetailsShowModal(parent, alternates[i].waypoint);
}
