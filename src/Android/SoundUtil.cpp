// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SoundUtil.hpp"
#include "java/Class.hxx"
#include "java/String.hxx"
#include "LocalPath.hpp"
#include "system/Path.hpp"

namespace SoundUtil {
static Java::TrivialClass cls;
static jmethodID play_method;
static jmethodID playExternal_method;
} // namespace SoundUtil

void
SoundUtil::Initialise(JNIEnv *env)
{
  assert(!cls.IsDefined());
  assert(env != nullptr);

  cls.Find(env, "org/xcsoar/SoundUtil");
  play_method = env->GetStaticMethodID(cls, "play",
                                       "(Landroid/content/Context;"
                                       "Ljava/lang/String;)Z");
  playExternal_method = env->GetStaticMethodID(cls, "playExternal",
                                               "(Landroid/content/Context;"
                                               "Ljava/lang/String;)Z");
}

void
SoundUtil::Deinitialise(JNIEnv *env)
{
  cls.Clear(env);
}

bool
SoundUtil::Play(JNIEnv *env, jobject context, const char *name)
{
  Java::String paramName(env, name);
  return env->CallStaticBooleanMethod(cls, play_method, context,
                                      paramName.Get());
}

bool
SoundUtil::PlayExternal(JNIEnv *env, jobject context, const char *path)
{
  AllocatedPath absolutePath = LocalPath(_T(path));
  Java::String paramName(env, absolutePath.c_str());
  return env->CallStaticBooleanMethod(cls, playExternal_method, context,
                                      paramName.Get());
}
