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

#ifndef XCSOAR_ANDROID_DOWNLOAD_MANAGER_HPP
#define XCSOAR_ANDROID_DOWNLOAD_MANAGER_HPP

#include "Java/Object.hxx"
#include "Thread/Mutex.hxx"

#include <list>

class Path;
class Context;

namespace Net {
  class DownloadListener;
}

class AndroidDownloadManager {
  Java::GlobalObject object;

  /**
   * Protects the #listeners attribute.
   */
  Mutex mutex;

  std::list<Net::DownloadListener *> listeners;

  AndroidDownloadManager(JNIEnv *env, jobject obj):object(env, obj) {}

public:
  static bool Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  gcc_pure
  static bool IsAvailable();

  static AndroidDownloadManager *Create(JNIEnv *env, Context &context);

  void AddListener(Net::DownloadListener &listener);
  void RemoveListener(Net::DownloadListener &listener);
  void OnDownloadComplete(Path path_relative, bool success);

  void Enumerate(JNIEnv *env, Net::DownloadListener &listener);
  void Enqueue(JNIEnv *env, const char *uri, Path path_relative);
  void Cancel(JNIEnv *env, Path path_relative);
};

#endif
