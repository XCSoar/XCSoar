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

#include "DownloadManager.hpp"
#include "Main.hpp"
#include "net/http/DownloadManager.hpp"
#include "Context.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "org_xcsoar_DownloadUtil.h"

#include <algorithm>

#include <windef.h> /* for MAX_PATH */

static AndroidDownloadManager *instance;

static Java::TrivialClass util_class;

static jmethodID enumerate_method, enqueue_method, cancel_method;

bool
AndroidDownloadManager::Initialise(JNIEnv *env) noexcept
{
  assert(util_class == nullptr);
  assert(env != nullptr);

  if (!util_class.FindOptional(env, "org/xcsoar/DownloadUtil"))
    return false;

  enumerate_method = env->GetStaticMethodID(util_class, "enumerate",
                                            "(Landroid/app/DownloadManager;J)V");

  enqueue_method = env->GetStaticMethodID(util_class, "enqueue",
                                          "(Landroid/app/DownloadManager;"
                                          "Ljava/lang/String;Ljava/lang/String;)J");

  cancel_method = env->GetStaticMethodID(util_class, "cancel",
                                         "(Landroid/app/DownloadManager;"
                                         "Ljava/lang/String;)V");

  return true;
}

void
AndroidDownloadManager::Deinitialise(JNIEnv *env) noexcept
{
  util_class.ClearOptional(env);
}

bool
AndroidDownloadManager::IsAvailable() noexcept
{
  return util_class.Get() != nullptr;
}

AndroidDownloadManager *
AndroidDownloadManager::Create(JNIEnv *env, Context &context) noexcept
{
  const auto obj = context.GetSystemService(env, "download");
  if (obj == nullptr)
    return nullptr;

  return instance = new AndroidDownloadManager(env, obj);
}

void
AndroidDownloadManager::AddListener(Net::DownloadListener &listener) noexcept
{
  std::lock_guard<Mutex> lock(mutex);

  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());

  listeners.push_back(&listener);
}

void
AndroidDownloadManager::RemoveListener(Net::DownloadListener &listener) noexcept
{
  std::lock_guard<Mutex> lock(mutex);

  auto i = std::find(listeners.begin(), listeners.end(), &listener);
  assert(i != listeners.end());
  listeners.erase(i);
}

void
AndroidDownloadManager::OnDownloadComplete(Path path_relative,
                                           bool success) noexcept
{
  std::lock_guard<Mutex> lock(mutex);

  if (success)
    for (auto i = listeners.begin(), end = listeners.end(); i != end; ++i)
      (*i)->OnDownloadComplete(path_relative);
  else
    for (auto i = listeners.begin(), end = listeners.end(); i != end; ++i)
      // TODO obtain error details
      (*i)->OnDownloadError(path_relative, {});
}

[[gnu::pure]]
static AllocatedPath
EraseSuffix(Path p, const char *suffix) noexcept
{
  assert(p != nullptr);
  assert(suffix != nullptr);

  const auto current_suffix = p.GetExtension();
  return current_suffix != nullptr && StringIsEqual(suffix, current_suffix)
    ? AllocatedPath(p.c_str(), current_suffix)
    : nullptr;
}

JNIEXPORT void JNICALL
Java_org_xcsoar_DownloadUtil_onDownloadAdded(JNIEnv *env, jclass cls,
                                             jlong j_handler, jstring j_path,
                                             jlong size, jlong position)
{
  char tmp_path[MAX_PATH];
  Java::String::CopyTo(env, j_path, tmp_path, ARRAY_SIZE(tmp_path));

  const auto final_path = EraseSuffix(Path(tmp_path), ".tmp");
  if (final_path == nullptr)
    return;

  const auto relative = RelativePath(final_path);
  if (relative == nullptr)
    return;

  Net::DownloadListener &handler = *(Net::DownloadListener *)(size_t)j_handler;
  handler.OnDownloadAdded(relative, size, position);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_DownloadUtil_onDownloadComplete(JNIEnv *env, jclass cls,
                                                jstring j_path,
                                                jboolean success)
{
  if (instance == nullptr)
    return;

  char tmp_path[MAX_PATH];
  Java::String::CopyTo(env, j_path, tmp_path, ARRAY_SIZE(tmp_path));

  const auto final_path = EraseSuffix(Path(tmp_path), ".tmp");
  if (final_path == nullptr)
    return;

  const auto relative = RelativePath(final_path);
  if (relative == nullptr)
    return;

  success = success && File::Replace(Path(tmp_path), final_path);

  instance->OnDownloadComplete(relative, success);
}

void
AndroidDownloadManager::Enumerate(JNIEnv *env,
                                  Net::DownloadListener &listener) noexcept
{
  assert(env != nullptr);

  env->CallStaticVoidMethod(util_class, enumerate_method,
                            object.Get(), (jlong)(size_t)&listener);
}

void
AndroidDownloadManager::Enqueue(JNIEnv *env, const char *uri,
                                Path path_relative) noexcept
{
  assert(env != nullptr);
  assert(uri != nullptr);
  assert(path_relative != nullptr);

  const auto tmp_absolute = LocalPath(path_relative) + ".tmp";
  File::Delete(tmp_absolute);

  Java::String j_uri(env, uri);
  Java::String j_path(env, tmp_absolute.c_str());

  env->CallStaticLongMethod(util_class, enqueue_method,
                            object.Get(), j_uri.Get(),
                            j_path.Get());

  try {
    /* the method DownloadManager.enqueue() can throw
       SecurityException if Android doesn't like the destination path
       ("Unsupported path") */
    Java::RethrowException(env);
  } catch (...) {
    const auto error = std::current_exception();

    std::lock_guard<Mutex> lock(mutex);
    for (auto *i : listeners)
      i->OnDownloadError(path_relative, error);
    return;
  }

  std::lock_guard<Mutex> lock(mutex);
  for (auto i = listeners.begin(), end = listeners.end(); i != end; ++i)
    (*i)->OnDownloadAdded(path_relative, -1, -1);
}

void
AndroidDownloadManager::Cancel(JNIEnv *env, Path path_relative) noexcept
{
  assert(env != nullptr);
  assert(path_relative != nullptr);

  const auto tmp_absolute = LocalPath(path_relative) + ".tmp";

  Java::String j_path(env, tmp_absolute.c_str());
  env->CallStaticVoidMethod(util_class, cancel_method,
                            object.Get(), j_path.Get());
}
