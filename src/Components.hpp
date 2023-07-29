// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FileCache;
class AsyncTerrainOverviewLoader;
class GlideComputer;
class DrawThread;
class MultipleDevices;
class DeviceBlackboard;
class MergeThread;
class CalculationThread;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;
class Replay;
class Logger;
class NMEALogger;
class GlueFlightLogger;
class TrackingGlue;
namespace TIM { class Glue; }

inline struct DataComponents *data_components;

// other global objects
extern FileCache *file_cache;
extern ProtectedTaskManager *protected_task_manager;
extern Replay *replay;
extern AsyncTerrainOverviewLoader *terrain_loader;
extern GlideComputer *glide_computer;
#ifndef ENABLE_OPENGL
extern DrawThread *draw_thread;
#endif
extern MultipleDevices *devices;
extern DeviceBlackboard *device_blackboard;
extern MergeThread *merge_thread;
extern CalculationThread *calculation_thread;

extern Logger *logger;
extern NMEALogger *nmea_logger;
extern GlueFlightLogger *flight_logger;

extern TrackingGlue *tracking;
extern TIM::Glue *tim_glue;

/**
 * Returns the global ProtectedAirspaceWarningManager instance.  May
 * be nullptr if disabled.
 */
[[gnu::pure]]
ProtectedAirspaceWarningManager *
GetAirspaceWarnings();
