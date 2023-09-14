// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "java/Closeable.hxx"

#include <cstddef>
#include <span>

class PortListener;
class DataHandler;

class PortBridge : protected Java::GlobalCloseable {
  static jmethodID setListener_method;
  static jmethodID setInputListener_method;
  static jmethodID getState_method;
  static jmethodID drain_method;
  static jmethodID getBaudRate_method, setBaudRate_method;
  static jmethodID write_method;

  static constexpr size_t write_buffer_size = 4096;
  Java::GlobalRef<jbyteArray> write_buffer;

public:
  /**
   * Global initialisation.  Looks up the methods of the AndroidPort
   * Java class.
   */
  static void Initialise(JNIEnv *env);

  PortBridge(JNIEnv *env, jobject obj);

  void setListener(JNIEnv *env, PortListener *listener);
  void setInputListener(JNIEnv *env, DataHandler *handler);

  int getState(JNIEnv *env) {
    return env->CallIntMethod(Get(), getState_method);
  }

  bool drain(JNIEnv *env) {
    return env->CallBooleanMethod(Get(), drain_method);
  }

  int getBaudRate(JNIEnv *env) const {
    return env->CallIntMethod(Get(), getBaudRate_method);
  }

  bool setBaudRate(JNIEnv *env, int baud_rate) {
    return env->CallBooleanMethod(Get(), setBaudRate_method, baud_rate);
  }

  std::size_t write(JNIEnv *env, std::span<const std::byte> src);
};
