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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Screen/Layout.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/WaypointListRenderer.hpp"
#include "Language/Language.hpp"

class AlternatesListDialog : public ListItemRenderer {
public:
  AbortTask::AlternateVector alternates;

  void Update() {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    alternates = lease->GetAlternates();
  }

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned index) gcc_override {
    assert(index < alternates.size());

    const ComputerSettings &settings = CommonInterface::GetComputerSettings();
    const Waypoint &waypoint = alternates[index].waypoint;
    const GlideResult& solution = alternates[index].solution;

    WaypointListRenderer::Draw(canvas, rc, waypoint, solution.vector.distance,
                               solution.SelectAltitudeDifference(settings.task.glide),
                               UIGlobals::GetDialogLook(),
                               UIGlobals::GetMapLook().waypoint,
                               CommonInterface::GetMapSettings().waypoint);
  }
};

void
dlgAlternatesListShowModal(SingleWindow &parent)
{
  if (protected_task_manager == NULL)
    return;

  AlternatesListDialog dialog;
  dialog.Update();

  const DialogLook &look = UIGlobals::GetDialogLook();
  int i = ListPicker(parent, _("Alternates"), dialog.alternates.size(), 0,
                     WaypointListRenderer::GetHeight(look),
                     dialog, true);

  if (i < 0 || (unsigned)i >= dialog.alternates.size())
    return;

  dlgWaypointDetailsShowModal(parent, dialog.alternates[i].waypoint);
}
