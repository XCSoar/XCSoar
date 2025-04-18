// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"

#include <forward_list>

class AllocatedPath;

class Context : public Java::GlobalObject {
public:
  /**
   * Global initialisation.  Looks up the methods of the
   * Context Java class.
   */
  static void Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  using Java::GlobalObject::GlobalObject;

  AllocatedPath GetExternalFilesDir(JNIEnv *env) noexcept;
  std::forward_list<AllocatedPath> GetExternalFilesDirs(JNIEnv *env) const noexcept;
  std::forward_list<AllocatedPath> GetExternalMediaDirs(JNIEnv *env) const noexcept;

  AllocatedPath GetExternalCacheDir(JNIEnv *env) noexcept;

  Java::LocalObject GetSystemService(JNIEnv *env, jstring name);
  Java::LocalObject GetSystemService(JNIEnv *env, const char *name);
  Java::LocalObject GetVibrator(JNIEnv *env);
};
