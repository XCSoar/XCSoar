// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/OutputStream.hxx"
#include "java/Ref.hxx"

#include <jni.h>

/**
 * An #OutputStream implementation that writes to a Java OutputStream
 * obtained via SAF (Android's Storage Access Framework).
 *
 * The underlying Java OutputStream is closed in the destructor.
 */
class SAFOutputStream final : public OutputStream {
  Java::GlobalRef<jobject> output_stream_;

  static jmethodID write_method_;
  static jmethodID close_method_;
  static jmethodID flush_method_;

public:
  /**
   * Adopt a Java OutputStream.  A new global reference is created
   * from the given local/global reference.
   */
  SAFOutputStream(JNIEnv *env, jobject output_stream) noexcept;
  ~SAFOutputStream() noexcept;

  void Commit() override;

  static void Initialise(JNIEnv *env) noexcept;

  /* virtual methods from class OutputStream */
  void Write(std::span<const std::byte> src) override;

  /** Flush and close the underlying Java stream explicitly. */
  void Close();
};
