// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TargetMapWindow.hpp"
#include "Look/TaskLook.hpp"
#include "ui/canvas/Icon.hpp"
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

  const GeoPoint gp = projection.ScreenToGeo(pc);
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
