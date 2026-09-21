// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OpenAssociatedFile.hpp"

#ifdef HAVE_RUN_FILE

#include "Engine/Waypoint/Waypoint.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "java/Global.hxx"
#endif

#include <iterator>

void
OpenWaypointAssociatedFile(const Waypoint &waypoint,
                           unsigned index) noexcept
{
  auto file = waypoint.files_external.begin();
  std::advance(file, index);

#ifdef ANDROID
  if (native_view != nullptr)
    native_view->OpenWaypointFile(Java::GetEnv(), waypoint.id,
                                  file->c_str());
#else
  RunFile(LocalPath(file->c_str()).c_str());
#endif
}

#endif
