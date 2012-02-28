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

#ifndef XCSOAR_ANDROID_PORT_BRIDGE_HPP
#define XCSOAR_ANDROID_PORT_BRIDGE_HPP

#include "Java/Object.hpp"

class PortBridge : protected Java::Object {
  jmethodID getBaudRate_method, setBaudRate_method;
  jmethodID setReadTimeout_mid;
  jmethodID read_mid, write_mid, flush_mid;
  jmethodID waitRead_method;

public:
  PortBridge(JNIEnv *env, jobject obj);

  ~PortBridge() {
    CallVoid("close");
  }

  int getBaudRate(JNIEnv *env) const {
    return env->CallIntMethod(Get(), getBaudRate_method);
  }

  int setBaudRate(JNIEnv *env, int baud_rate) {
    return env->CallIntMethod(Get(), setBaudRate_method, baud_rate);
  }

  void setReadTimeout(JNIEnv *env, int timeout) {
    env->CallVoidMethod(Get(), setReadTimeout_mid, timeout);
  }

  int read(JNIEnv *env) {
    return env->CallIntMethod(Get(), read_mid);
  }

  int waitRead(JNIEnv *env, unsigned timeout_ms) {
    return env->CallIntMethod(Get(), waitRead_method, timeout_ms);
  }

  bool write(JNIEnv *env, int ch) {
    return env->CallBooleanMethod(Get(), write_mid, ch);
  }

  void flush(JNIEnv *env) {
    env->CallVoidMethod(Get(), flush_mid);
  }
};

#endif
