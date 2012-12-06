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

#ifndef XCSOAR_WAYPOINT_COMMANDS_WIDGET_HPP
#define XCSOAR_WAYPOINT_COMMANDS_WIDGET_HPP

#include "Widget/RowFormWidget.hpp"
#include "Form/ActionListener.hpp"

class WndForm;
struct Waypoint;
class ProtectedTaskManager;

/**
 * A Widget that shows a few commands for a Waypoint.
 */
struct WaypointCommandsWidget
  : public RowFormWidget, ActionListener {
  WndForm *form;

  const Waypoint &waypoint;

  ProtectedTaskManager *task_manager;

public:
  WaypointCommandsWidget(const DialogLook &look, WndForm *_form,
                         const Waypoint &_waypoint,
                         ProtectedTaskManager *_task_manager)
    :RowFormWidget(look), form(_form),
     waypoint(_waypoint), task_manager(_task_manager) {}

  /* methods from ActionListener */
  virtual void OnAction(int id);

  /* methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
};

#endif
