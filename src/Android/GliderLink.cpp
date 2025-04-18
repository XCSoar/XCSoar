// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GliderLink.hpp"
#include "NativeSensorListener.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"
#include "Context.hpp"

static Java::TrivialClass gl_cls;
static jmethodID gl_ctor_id;

void
GliderLink::Initialise(JNIEnv *env) noexcept
{
  assert(!gl_cls.IsDefined());
  assert(env != nullptr);

  gl_cls.Find(env, "org/xcsoar/GliderLinkReceiver");

  gl_ctor_id = env->GetMethodID(gl_cls, "<init>",
                                 "(Landroid/content/Context;Lorg/xcsoar/SensorListener;)V");
}

void
GliderLink::Deinitialise(JNIEnv *env) noexcept
{
  gl_cls.Clear(env);
}

Java::LocalObject
GliderLink::Create(JNIEnv *env, Context &context, SensorListener &listener)
{
  return Java::NewObjectRethrow(env, gl_cls, gl_ctor_id, context.Get(),
                                NativeSensorListener::Create(env, listener).Get());
}
