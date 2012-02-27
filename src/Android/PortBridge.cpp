/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Android/PortBridge.hpp"
#include "Java/Class.hpp"

#include <string.h>

PortBridge::PortBridge(JNIEnv *env, jobject obj)
  :Java::Object(env, obj) {
  Java::Class cls(env, env->GetObjectClass(obj));
  drain_method = env->GetMethodID(cls, "drain", "()Z");
  getBaudRate_method = env->GetMethodID(cls, "getBaudRate", "()I");
  setBaudRate_method = env->GetMethodID(cls, "setBaudRate", "(I)Z");
  setReadTimeout_mid = env->GetMethodID(cls, "setReadTimeout", "(I)V");
  read_method = env->GetMethodID(cls, "read", "([BI)I");
  write_method = env->GetMethodID(cls, "write", "([BI)I");
  flush_mid = env->GetMethodID(cls, "flush", "()V");
  waitRead_method = env->GetMethodID(cls, "waitRead", "(I)I");

  read_buffer.Set(env, env->NewByteArray(read_buffer_size));
  write_buffer.Set(env, env->NewByteArray(write_buffer_size));
}

int
PortBridge::read(JNIEnv *env, void *buffer, size_t length)
{
  if (length > read_buffer_size)
    length = read_buffer_size;

  jint nbytes = env->CallIntMethod(Get(), read_method,
                                   read_buffer.Get(), length);
  if (nbytes > 0) {
    jbyte *src = env->GetByteArrayElements(read_buffer, NULL);
    memcpy(buffer, src, nbytes);
    env->ReleaseByteArrayElements(read_buffer, src, 0);
  }

  return nbytes;
}

int
PortBridge::write(JNIEnv *env, const void *data, size_t length)
{
  if (length > write_buffer_size)
    length = write_buffer_size;

  jbyte *dest = env->GetByteArrayElements(write_buffer, NULL);
  memcpy(dest, data, length);
  env->ReleaseByteArrayElements(write_buffer, dest, 0);

  return env->CallIntMethod(Get(), write_method, write_buffer.Get(), length);
}
