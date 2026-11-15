// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class FileCache;
class AsyncTerrainOverviewLoader;
class DrawThread;
class UpdateService;

inline struct NetComponents *net_components;
inline struct DataComponents *data_components;
inline struct BackendComponents *backend_components;

/** Application-level update coordinator and owner of all update providers. */
extern std::unique_ptr<UpdateService> update_service;

// other global objects
extern FileCache *file_cache;
extern AsyncTerrainOverviewLoader *terrain_loader;
#ifndef ENABLE_OPENGL
extern DrawThread *draw_thread;
#endif
