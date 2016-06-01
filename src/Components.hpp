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

#ifndef XCSOAR_COMPONENTS_HPP
#define XCSOAR_COMPONENTS_HPP

#include "Compiler.h"

class FileCache;
class TopographyStore;
class RasterTerrain;
class GlideComputer;
class DrawThread;
class MultipleDevices;
class DeviceBlackboard;
class MergeThread;
class CalculationThread;
class Waypoints;
class Airspaces;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;
class Replay;
class Logger;
class GlueFlightLogger;
class TrackingGlue;

// other global objects
extern FileCache *file_cache;
extern Airspaces airspace_database;
extern Waypoints way_points;
extern ProtectedTaskManager *protected_task_manager;
extern Replay *replay;
extern TopographyStore *topography;
extern RasterTerrain *terrain;
extern GlideComputer *glide_computer;
#ifndef ENABLE_OPENGL
extern DrawThread *draw_thread;
#endif
extern MultipleDevices *devices;
extern DeviceBlackboard *device_blackboard;
extern MergeThread *merge_thread;
extern CalculationThread *calculation_thread;

extern Logger *logger;
extern GlueFlightLogger *flight_logger;

extern TrackingGlue *tracking;

/**
 * Returns the global ProtectedAirspaceWarningManager instance.  May
 * be nullptr if disabled.
 */
gcc_pure
ProtectedAirspaceWarningManager *
GetAirspaceWarnings();

#endif
