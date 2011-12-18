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

#include "InputEvents.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Widgets/TrafficWidget.hpp"

/**
 * Evil global variable - please refactor!
 */
static TrafficWidget *traffic_widget;

void
InputEvents::eventTraffic(const TCHAR *misc)
{
  if (_tcscmp(misc, _T("show")) == 0) {
    if (!CommonInterface::Basic().flarm.available ||
        IsFlavour(_T("Traffic")))
      return;

    traffic_widget = new TrafficWidget();
    CommonInterface::main_window.SetWidget(traffic_widget);
    SetFlavour(_T("Traffic"));
    return;
  }

  if (!IsFlavour(_T("Traffic")))
    return;

  assert(traffic_widget != NULL);

  if (_tcscmp(misc, _T("zoom auto toggle")) == 0) {
    traffic_widget->ToggleAutoZoom();
  } else if (_tcscmp(misc, _T("zoom in")) == 0) {
    traffic_widget->ZoomIn();
  } else if (_tcscmp(misc, _T("zoom out")) == 0) {
    traffic_widget->ZoomOut();
  } else if (_tcscmp(misc, _T("northup toggle")) == 0) {
    traffic_widget->ToggleNorthUp();
  }
}
