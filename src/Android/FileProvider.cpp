// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "org_xcsoar_FileProvider.h"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/Path.hpp"
#include "java/String.hxx"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "LocalPath.hpp"

#include <cassert>

JNIEXPORT jstring JNICALL
Java_org_xcsoar_FileProvider_getWaypointFileForUri(JNIEnv *env, jclass,
                                                   jint id, jstring _filename)
{
  auto w = data_components->waypoints->LookupId(id);
  if (!w)
    return nullptr;

  const auto filename = Java::String::GetUTFChars(env, _filename);

  /* check if the given file really exists; refuse access to other
     files not specified in the waypoint details file */
  for (const auto &i : w->files_external) {
    if (i == filename.c_str()) {
      auto path = LocalPath(filename.c_str());
      return env->NewStringUTF(path.c_str());
    }
  }

  return nullptr;
}
