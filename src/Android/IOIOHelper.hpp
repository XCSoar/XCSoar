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

#ifndef XCSOAR_ANDROID_IOIOHELPER_HPP
#define XCSOAR_ANDROID_IOIOHELPER_HPP

#include "Java/Object.hpp"
#include "Java/Ref.hpp"

class IOIOHelper : protected Java::Object {
  Java::GlobalRef<jclass> cls;
  jmethodID openUart_mid, resetInputThread_mid;
  jmethodID setReadTimeout_mid;
  jmethodID setBaudRate_mid, getBaudRate_mid;
  jmethodID read_mid, write_mid, flush_mid;

  IOIOHelper(JNIEnv *env, jclass _cls, jobject obj)
    :Java::Object(env, obj), cls(env, _cls) {
    openUart_mid = env->GetMethodID(cls, "openUart", "(II)I");
    setReadTimeout_mid = env->GetMethodID(cls, "setReadTimeout", "(II)V");
    setBaudRate_mid = env->GetMethodID(cls, "setBaudRate", "(II)I");
    getBaudRate_mid = env->GetMethodID(cls, "getBaudRate", "(I)I");
    read_mid = env->GetMethodID(cls, "read", "(I)I");
    write_mid = env->GetMethodID(cls, "write", "(IB)V");
    flush_mid = env->GetMethodID(cls, "flush", "(I)V");
  }

public:
  ~IOIOHelper() {
    call_void("close");
  }

  gcc_malloc
  static IOIOHelper *connect(JNIEnv *env);

  int openUart(JNIEnv *env, unsigned ID, unsigned baud) {
    return env->CallIntMethod(get(), openUart_mid, ID, (int)baud);
  }

  unsigned setBaudRate(JNIEnv *env, unsigned ID, unsigned baud) {
    return env->CallIntMethod(get(), setBaudRate_mid, ID, (int)baud);
  }

  unsigned getBaudRate(JNIEnv *env, unsigned ID) {
    return (unsigned)(env->CallIntMethod(get(), getBaudRate_mid, ID));
  }

  void setReadTimeout(JNIEnv *env, unsigned ID, int timeout) {
    env->CallVoidMethod(get(), setReadTimeout_mid, ID, timeout);
  }

  int read(JNIEnv *env, unsigned ID) {
    return env->CallIntMethod(get(), read_mid, ID);
  }

  bool write(JNIEnv *env, unsigned ID, int ch) {
    env->CallVoidMethod(get(), write_mid, ID, ch);
    return true;
  }

  void flush(JNIEnv *env, unsigned ID) {
    env->CallVoidMethod(get(), flush_mid, ID);
  }
};

#endif
