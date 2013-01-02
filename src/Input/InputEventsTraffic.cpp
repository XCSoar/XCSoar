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

#include "InputEvents.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Widgets/TrafficWidget.hpp"
#include "FLARM/Glue.hpp"

/**
 * Evil global variable - please refactor!
 */
static TrafficWidget *traffic_widget;

void
InputEvents::eventTraffic(const TCHAR *misc)
{
  LoadFlarmDatabases();

  if (StringIsEqual(misc, _T("show"))) {
    if (CommonInterface::Basic().flarm.traffic.IsEmpty() ||
        IsFlavour(_T("Traffic")))
      return;

    traffic_widget = new TrafficWidget();
    CommonInterface::main_window->SetWidget(traffic_widget);
    SetFlavour(_T("Traffic"));
    return;
  }

  if (!IsFlavour(_T("Traffic")))
    return;

  assert(traffic_widget != NULL);

  if (StringIsEqual(misc, _T("zoom auto toggle"))) {
    traffic_widget->ToggleAutoZoom();
  } else if (StringIsEqual(misc, _T("zoom in"))) {
    traffic_widget->ZoomIn();
  } else if (StringIsEqual(misc, _T("zoom out"))) {
    traffic_widget->ZoomOut();
  } else if (StringIsEqual(misc, _T("northup toggle"))) {
    traffic_widget->ToggleNorthUp();
  }
}
