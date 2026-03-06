// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/Reader.hxx"
#include "java/Ref.hxx"

#include <jni.h>

/**
 * A #Reader implementation that reads from a Java InputStream
 * obtained via SAF (Android's Storage Access Framework).
 *
 * Uses Java::InputStream for JNI method IDs.
 * The InputStream is closed in the destructor.
 */
class SAFReader final : public Reader {
  Java::GlobalRef<jobject> input_stream_;

public:
  /**
   * Adopt a Java InputStream.  A new global reference is created
   * from the given local/global reference.
   */
  SAFReader(JNIEnv *env, jobject input_stream) noexcept;
  ~SAFReader() noexcept override;

  /* virtual methods from class Reader */
  [[nodiscard]]
  std::size_t Read(std::span<std::byte> dest) override;
};
