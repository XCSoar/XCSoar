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

#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Thread/Debug.hpp"

FileCache *file_cache;
TopographyStore *topography;
RasterTerrain *terrain;

#ifndef ENABLE_OPENGL
DrawThread *draw_thread;
#endif

MultipleDevices *devices;
DeviceBlackboard *device_blackboard;

MergeThread *merge_thread;
CalculationThread *calculation_thread;

Logger *logger;
GlueFlightLogger *flight_logger;
Replay *replay;

#ifdef HAVE_TRACKING
TrackingGlue *tracking;
#endif

Waypoints way_points;

ProtectedTaskManager *protected_task_manager;

Airspaces airspace_database;

GlideComputer *glide_computer;

ProtectedAirspaceWarningManager *
GetAirspaceWarnings()
{
  return glide_computer != nullptr
    ? &glide_computer->GetAirspaceWarnings()
    : nullptr;
}

#ifndef NDEBUG

#ifdef ENABLE_OPENGL

static const ThreadHandle zero_thread_handle = ThreadHandle();
static ThreadHandle draw_thread_handle;

bool
InDrawThread()
{
#ifdef ENABLE_OPENGL
  return InMainThread() && draw_thread_handle.IsInside();
#else
  return draw_thread != nullptr && draw_thread->IsInside();
#endif
}

void
EnterDrawThread()
{
  assert(InMainThread());
  assert(draw_thread_handle == zero_thread_handle);

  draw_thread_handle = ThreadHandle::GetCurrent();
}

void
LeaveDrawThread()
{
  assert(InMainThread());
  assert(draw_thread_handle.IsInside());

  draw_thread_handle = zero_thread_handle;
}

#else

#include "DrawThread.hpp"

bool
InDrawThread()
{
  return draw_thread != nullptr && draw_thread->IsInside();
}

#endif

#endif
