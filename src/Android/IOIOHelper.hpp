// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Object.hxx"
#include "java/Class.hxx"

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

  /**
   * Throws on error.
   */
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
