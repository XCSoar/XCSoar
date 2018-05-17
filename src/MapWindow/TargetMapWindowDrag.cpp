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

#include "TargetMapWindow.hpp"
#include "Look/TaskLook.hpp"
#include "Screen/Icon.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Engine/Task/ObservationZones/ObservationZonePoint.hpp"
#include "Screen/Layout.hpp"

void
TargetMapWindow::OnTaskModified()
{
  Invalidate();
}

void
TargetMapWindow::TargetPaintDrag(Canvas &canvas, const PixelPoint drag_last)
{
  task_look.target_icon.Draw(canvas, drag_last);
}

bool
TargetMapWindow::TargetDragged(PixelPoint p)
{
  assert(task != nullptr);

  GeoPoint gp = projection.ScreenToGeo(p);

  {
    ProtectedTaskManager::ExclusiveLease task_manager(*task);
    if (!task_manager->TargetIsLocked(target_index))
      task_manager->TargetLock(target_index, true);

    task_manager->SetTarget(target_index, gp, true);
  }

  OnTaskModified();
  return true;
}

bool
TargetMapWindow::isClickOnTarget(const PixelPoint pc) const
{
  if (task == nullptr)
    return false;

  ProtectedTaskManager::Lease task_manager(*task);
  const GeoPoint t = task_manager->GetLocationTarget(target_index);
  if (!t.IsValid())
    return false;

  const GeoPoint gp = projection.ScreenToGeo(pc.x, pc.y);
  if (projection.GeoToScreenDistance(gp.DistanceS(t)) < Layout::GetHitRadius())
    return true;

  return false;
}

bool
TargetMapWindow::isInSector(PixelPoint pt)
{
  assert(task != nullptr);

  GeoPoint gp = projection.ScreenToGeo(pt);

  ProtectedTaskManager::Lease lease(*task);
  AATPoint *p = lease->GetOrderedTask().GetAATTaskPoint(target_index);
  return p != nullptr && p->GetObservationZone().IsInSector(gp);
}
