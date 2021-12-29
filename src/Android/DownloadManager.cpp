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
#include "java/Env.hxx"
#include "java/Path.hxx"
#include "java/String.hxx"
#include "LocalPath.hpp"
#include "io/CopyFile.hxx"
#include "util/Macros.hpp"
#include "util/StringAPI.hxx"
#include "org_xcsoar_DownloadUtil.h"

#include <algorithm>

static Java::TrivialClass util_class;

static jmethodID ctor, enumerate_method, enqueue_method, cancel_method;

static Java::LocalObject
NewDownloadUtil(JNIEnv *env, AndroidDownloadManager &instance, Context &context)
{
  return Java::NewObjectRethrow(env, util_class, ctor,
                                (jlong)(std::size_t)&instance,
                                context.Get());
}

AndroidDownloadManager::AndroidDownloadManager(JNIEnv *env,
                                               Context &context)
  :util(env, NewDownloadUtil(env, *this, context))
{
}

bool
AndroidDownloadManager::Initialise(JNIEnv *env) noexcept
{
  assert(util_class == nullptr);
  assert(env != nullptr);

  if (!util_class.FindOptional(env, "org/xcsoar/DownloadUtil"))
    return false;

  ctor = env->GetMethodID(util_class, "<init>",
                          "(JLandroid/content/Context;)V");
  if (Java::DiscardException(env)) {
    /* need to check for Java exceptions again because the first
       method lookup initializes the Java class */
    util_class.Clear(env);
    return false;
  }

  enumerate_method = env->GetMethodID(util_class, "enumerate", "(J)V");

  enqueue_method = env->GetMethodID(util_class, "enqueue",
                                    "(Ljava/lang/String;Ljava/lang/String;)J");

  cancel_method = env->GetMethodID(util_class, "cancel",
                                   "(Ljava/lang/String;)V");

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

JNIEXPORT void JNICALL
Java_org_xcsoar_DownloadUtil_onDownloadAdded(JNIEnv *env, jobject obj,
                                             jlong j_handler, jstring j_path,
                                             jlong size, jlong position)
{
  const auto relative_path = Java::ToPath(env, j_path);

  Net::DownloadListener &handler = *(Net::DownloadListener *)(size_t)j_handler;
  handler.OnDownloadAdded(relative_path, size, position);
}

JNIEXPORT void JNICALL
Java_org_xcsoar_DownloadUtil_onDownloadComplete(JNIEnv *env, jobject obj,
                                                jlong ptr,
                                                jstring j_tmp_path,
                                                jstring j_relative_path,
                                                jboolean success)
{
  auto &dm = *(AndroidDownloadManager *)(size_t)ptr;

  const auto tmp_path = Java::ToPath(env, j_tmp_path);
  const auto relative_path = Java::ToPath(env, j_relative_path);

  const auto final_path = LocalPath(relative_path);

  if (success) {
    try {
      MoveOrCopyFile(tmp_path, final_path);
    } catch (...) {
      success = false;
    }
  }

  dm.OnDownloadComplete(relative_path, success);
}

void
AndroidDownloadManager::Enumerate(JNIEnv *env,
                                  Net::DownloadListener &listener) noexcept
{
  assert(env != nullptr);

  env->CallVoidMethod(util, enumerate_method,
                      (jlong)(size_t)&listener);
}

void
AndroidDownloadManager::Enqueue(JNIEnv *env, const char *uri,
                                Path path_relative) noexcept
{
  assert(env != nullptr);
  assert(uri != nullptr);
  assert(path_relative != nullptr);

  Java::String j_uri(env, uri);
  Java::String j_path(env, path_relative.c_str());

  env->CallLongMethod(util, enqueue_method,
                      j_uri.Get(), j_path.Get());

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

  Java::String j_path(env, path_relative.c_str());
  env->CallVoidMethod(util, cancel_method, j_path.Get());
}
