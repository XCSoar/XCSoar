/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Java/Object.hxx"
#include "Java/Class.hxx"

class PortBridge;

class IOIOHelper : protected Java::GlobalObject {
private:
  static Java::TrivialClass cls;
  static jmethodID ctor;
  static jmethodID open_method;
  static jmethodID openUart_method;
  static jmethodID shutdown_method;

public:
  /**
   * Global initialisation.  Looks up the methods of the
   * IOIOHelper Java class.
   */
  static bool Initialise(JNIEnv *env);
  static void Deinitialise(JNIEnv *env);

  IOIOHelper(JNIEnv *env);

  ~IOIOHelper() {
    shutdown(Java::GetEnv());
  }

  jobject GetHolder() {
    return Java::GlobalObject::Get();
  }

  /**
   * Close the connection to the IOIO board.  It can be reopened
   * by calling open().
   * Should not be called until all open Uarts have been closed
   * by calling closeUart();
   * @param env
   */
  void shutdown(JNIEnv *env) {
    env->CallVoidMethod(Get(), shutdown_method);
  }

  /**
   * Opens a single Uart.  Can only be called after connect()
   * has been called.
   * @param env
   * @param ID Id of Uart (0, 1, 2, 3)
   * @param baud
   * @return True if Uart port on IOIO board is successfully
   * opened.  Else false.
   */
  PortBridge *openUart(JNIEnv *env, unsigned ID, unsigned baud);
};

#endif
