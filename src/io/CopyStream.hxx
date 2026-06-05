// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Reader.hxx"
#include "OutputStream.hxx"
#include "Operation/Operation.hpp"
#include "Operation/Cancelled.hpp"

#include <cstdint>
#include <span>

/**
 * Copy all bytes from a Reader to an OutputStream using a 64 KiB
 * buffer.  Returns when the Reader reaches end-of-stream.
 *
 * Throws on I/O errors or if the operation is cancelled.
 *
 * @param env optional environment for cancellation and progress
 * @param total_size if > 0 and env is set, report byte-level
 *   progress (range = total_size / buffer_size)
 */
inline void
CopyStream(Reader &in, OutputStream &out,
           OperationEnvironment *env = nullptr,
           uint_least64_t total_size = 0)
{
  constexpr std::size_t buffer_size = 65536;
  std::byte buffer[buffer_size];

  if (env && total_size > 0) {
    const unsigned range = (total_size + buffer_size - 1) / buffer_size;
    env->SetProgressRange(range);
    env->SetProgressPosition(0);
  }

  unsigned position = 0;
  while (true) {
    if (env && env->IsCancelled())
      throw OperationCancelled{};
    auto n = in.Read(buffer);
    if (n == 0)
      break;
    out.Write(std::span{buffer}.first(n));

    if (env && total_size > 0)
      env->SetProgressPosition(++position);
  }
}
