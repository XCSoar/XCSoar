// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Closeable.hxx"
#include "thread/Mutex.hxx"

#include <list>

class Path;
class Context;
namespace Net { class DownloadListener; }

class AndroidDownloadManager {
  Java::GlobalCloseable util;

  /**
   * Protects the #listeners attribute.
   */
  Mutex mutex;

  std::list<Net::DownloadListener *> listeners;

public:
  /**
   * Throws on error.
   */
  AndroidDownloadManager(JNIEnv *env, Context &context);

  static bool Initialise(JNIEnv *env) noexcept;
  static void Deinitialise(JNIEnv *env) noexcept;

  [[gnu::pure]]
  static bool IsAvailable() noexcept;

  void AddListener(Net::DownloadListener &listener) noexcept;
  void RemoveListener(Net::DownloadListener &listener) noexcept;
  void OnDownloadComplete(Path path_relative, bool success) noexcept;

  void Enumerate(JNIEnv *env, Net::DownloadListener &listener) noexcept;
  void Enqueue(JNIEnv *env, const char *uri, Path path_relative) noexcept;
  void Cancel(JNIEnv *env, Path path_relative) noexcept;
};
