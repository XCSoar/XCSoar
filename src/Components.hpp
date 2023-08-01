// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FileCache;
class AsyncTerrainOverviewLoader;
class DrawThread;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;
class Replay;
class TrackingGlue;
namespace TIM { class Glue; }

inline struct DataComponents *data_components;
inline struct BackendComponents *backend_components;

// other global objects
extern FileCache *file_cache;
extern ProtectedTaskManager *protected_task_manager;
extern Replay *replay;
extern AsyncTerrainOverviewLoader *terrain_loader;
#ifndef ENABLE_OPENGL
extern DrawThread *draw_thread;
#endif

extern TrackingGlue *tracking;
extern TIM::Glue *tim_glue;

/**
 * Returns the global ProtectedAirspaceWarningManager instance.  May
 * be nullptr if disabled.
 */
[[gnu::pure]]
ProtectedAirspaceWarningManager *
GetAirspaceWarnings();
