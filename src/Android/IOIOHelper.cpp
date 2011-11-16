/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Android/IOIOHelper.hpp"
#include "Java/Class.hpp"

IOIOHelper::IOIOHelper(JNIEnv *env)
{
  jclass _cls = env->FindClass("org/xcsoar/IOIOHelper");
  assert(_cls != NULL);

  const Java::Class cls(env, _cls);

  jmethodID cid = env->GetMethodID(cls, "<init>", "()V");
  assert(cid != NULL);

  jobject obj = env->NewObject(cls, cid);
  assert(obj != NULL);

  set(env, obj);

  env->DeleteLocalRef(obj);

  openUart_mid = env->GetMethodID(cls, "openUart", "(II)I");
  closeUart_mid = env->GetMethodID(cls, "closeUart", "(I)V");
  setReadTimeout_mid = env->GetMethodID(cls, "setReadTimeout", "(II)V");
  setBaudRate_mid = env->GetMethodID(cls, "setBaudRate", "(II)I");
  getBaudRate_mid = env->GetMethodID(cls, "getBaudRate", "(I)I");
  read_mid = env->GetMethodID(cls, "read", "(I)I");
  write_mid = env->GetMethodID(cls, "write", "(IB)V");
  flush_mid = env->GetMethodID(cls, "flush", "(I)V");
}
