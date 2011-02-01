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

#include "Android/BluetoothHelper.hpp"
#include "Java/String.hpp"
#include "Java/Class.hpp"

jobjectArray
BluetoothHelper::list(JNIEnv *env)
{
  jclass _cls = env->FindClass("org/xcsoar/BluetoothHelper");
  if (_cls == NULL) {
    /* Android < 2.0 doesn't have Bluetooth support */
    env->ExceptionClear();
    return NULL;
  }

  const Java::Class cls(env, _cls);

  /* call BluetoothHelper.connect() */

  jmethodID cid = env->GetStaticMethodID(cls, "list",
                                         "()[Ljava/lang/String;");
  assert(cid != NULL);

  return (jobjectArray)env->CallStaticObjectMethod(cls, cid);
}

BluetoothHelper *
BluetoothHelper::connect(JNIEnv *env, const char *address)
{
  jclass _cls = env->FindClass("org/xcsoar/BluetoothHelper");
  if (_cls == NULL) {
    /* Android < 2.0 doesn't have Bluetooth support */
    env->ExceptionClear();
    return NULL;
  }

  const Java::Class cls(env, _cls);

  /* call BluetoothHelper.connect() */

  jmethodID cid = env->GetStaticMethodID(cls, "connect",
                                         "(Ljava/lang/String;)Lorg/xcsoar/BluetoothHelper;");
  assert(cid != NULL);

  const Java::String address2(env, address);
  jobject obj = env->CallStaticObjectMethod(cls, cid, address2.get());
  if (obj == NULL)
    return NULL;

  BluetoothHelper *helper = new BluetoothHelper(env, cls, obj);
  env->DeleteLocalRef(obj);

  return helper;
}
