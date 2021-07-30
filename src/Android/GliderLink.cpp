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
