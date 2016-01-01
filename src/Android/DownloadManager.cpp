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

#include "DownloadManager.hpp"
#include "Main.hpp"
#include "Net/HTTP/DownloadManager.hpp"
#include "Context.hpp"
#include "Java/Class.hxx"
#include "Java/String.hxx"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Util/Macros.hpp"
#include "Util/StringAPI.hxx"
#include "org_xcsoar_DownloadUtil.h"

#include <algorithm>

#include <windef.h> /* for MAX_PATH */

static AndroidDownloadManager *instance;

static Java::TrivialClass util_class;

static jmethodID enumerate_method, enqueue_method, cancel_method;

bool
AndroidDownloadManager::Initialise(JNIEnv *env)
{
  assert(util_class == nullptr);
  assert(env != nullptr);

  if (android_api_level < 9 ||
      !util_class.FindOptional(env, "org/xcsoar/DownloadUtil"))
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
AndroidDownloadManager::Deinitialise(JNIEnv *env)
{
  util_class.ClearOptional(env);
}

bool
AndroidDownloadManager::IsAvailable()
{
  return util_class.Get() != nullptr;
}

AndroidDownloadManager *
AndroidDownloadManager::Create(JNIEnv *env, Context &context)
{
  jobject obj = context.GetSystemService(env, "download");
  if (obj == nullptr)
    return nullptr;

  instance = new AndroidDownloadManager(env, obj);
  env->DeleteLocalRef(obj);
  return instance;
}

void
AndroidDownloadManager::AddListener(Net::DownloadListener &listener)
{
  ScopeLock protect(mutex);

  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());

  listeners.push_back(&listener);
}

void
AndroidDownloadManager::RemoveListener(Net::DownloadListener &listener)
{
  ScopeLock protect(mutex);

  auto i = std::find(listeners.begin(), listeners.end(), &listener);
  assert(i != listeners.end());
  listeners.erase(i);
}

void
AndroidDownloadManager::OnDownloadComplete(Path path_relative,
                                           bool success)
{
  ScopeLock protect(mutex);
  for (auto i = listeners.begin(), end = listeners.end(); i != end; ++i)
    (*i)->OnDownloadComplete(path_relative, success);
}

gcc_pure
static AllocatedPath
EraseSuffix(Path p, const char *suffix)
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
AndroidDownloadManager::Enumerate(JNIEnv *env, Net::DownloadListener &listener)
{
  assert(env != nullptr);

  env->CallStaticVoidMethod(util_class, enumerate_method,
                            object.Get(), (jlong)(size_t)&listener);
}

void
AndroidDownloadManager::Enqueue(JNIEnv *env, const char *uri,
                                Path path_relative)
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

  ScopeLock protect(mutex);
  for (auto i = listeners.begin(), end = listeners.end(); i != end; ++i)
    (*i)->OnDownloadAdded(path_relative, -1, -1);
}

void
AndroidDownloadManager::Cancel(JNIEnv *env, Path path_relative)
{
  assert(env != nullptr);
  assert(path_relative != nullptr);

  const auto tmp_absolute = LocalPath(path_relative) + ".tmp";

  Java::String j_path(env, tmp_absolute.c_str());
  env->CallStaticVoidMethod(util_class, cancel_method,
                            object.Get(), j_path.Get());
}
