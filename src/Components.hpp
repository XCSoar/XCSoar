// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FileCache;
class AsyncTerrainOverviewLoader;
class DrawThread;

inline struct NetComponents *net_components;
inline struct DataComponents *data_components;
inline struct BackendComponents *backend_components;

// other global objects
extern FileCache *file_cache;
extern AsyncTerrainOverviewLoader *terrain_loader;
#ifndef ENABLE_OPENGL
extern DrawThread *draw_thread;
#endif
