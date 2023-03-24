// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Waypoint/Waypoints.hpp"
#include "net/http/Features.hpp"
#include "thread/Debug.hpp"
#include "thread/Handle.hpp"

FileCache *file_cache;
TopographyStore *topography;
RasterTerrain *terrain;
AsyncTerrainOverviewLoader *terrain_loader;

#ifndef ENABLE_OPENGL
DrawThread *draw_thread;
#endif

MultipleDevices *devices;
DeviceBlackboard *device_blackboard;

MergeThread *merge_thread;
CalculationThread *calculation_thread;

Logger *logger;
NMEALogger *nmea_logger;
GlueFlightLogger *flight_logger;
Replay *replay;

#ifdef HAVE_TRACKING
TrackingGlue *tracking;
#endif

#ifdef HAVE_HTTP
TIM::Glue *tim_glue;
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
