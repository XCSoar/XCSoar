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
private:
  jmethodID open_mid;
  jmethodID openUart_mid, closeUart_mid, resetInputThread_mid;
  jmethodID setReadTimeout_mid;
  jmethodID setBaudRate_mid, getBaudRate_mid;
  jmethodID read_mid, write_mid, flush_mid;

public:
  IOIOHelper(JNIEnv *env);
  ~IOIOHelper() {
    close();
  }

  bool open(JNIEnv *env) {
    return env->CallBooleanMethod(get(), open_mid);
  }

  void close() {
    call_void("close");
  }

  int openUart(JNIEnv *env, unsigned ID, unsigned baud) {
    return env->CallIntMethod(get(), openUart_mid, ID, (int)baud);
  }

  void closeUart(JNIEnv *env, unsigned ID) {
    env->CallVoidMethod(get(), closeUart_mid, ID);
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
