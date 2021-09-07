/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "org_xcsoar_FileProvider.h"
#include "Engine/Waypoint/Waypoints.hpp"
#include "system/Path.hpp"
#include "java/String.hxx"
#include "Components.hpp"
#include "LocalPath.hpp"

#include <cassert>

JNIEXPORT jstring JNICALL
Java_org_xcsoar_FileProvider_getWaypointFileForUri(JNIEnv *env, jclass,
                                                   jint id, jstring _filename)
{
  auto w = way_points.LookupId(id);
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
