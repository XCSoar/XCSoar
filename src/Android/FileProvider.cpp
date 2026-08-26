// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "org_xcsoar_FileProvider.h"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "java/String.hxx"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "LocalPath.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"

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

JNIEXPORT jstring JNICALL
Java_org_xcsoar_FileProvider_getLocalFileForUri(JNIEnv *env, jclass,
                                                jstring _relative_path)
{
  const auto relative = Java::String::GetUTFChars(env, _relative_path);
  const char *rel = relative.c_str();
  if (rel == nullptr || *rel == '\0')
    return nullptr;

  /* Refuse absolute paths and parent-directory traversal. */
  if (*rel == '/' || *rel == '\\' ||
      StringFind(rel, "..") != nullptr)
    return nullptr;

  const auto path = LocalPath(rel);
  if (!File::Exists(path))
    return nullptr;

  return env->NewStringUTF(path.c_str());
}
