/*
 * Copyright (C) 2012 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_EVENT_PIPE_HPP
#define XCSOAR_EVENT_PIPE_HPP

#include "FileDescriptor.hpp"

#include <assert.h>

/**
 * This class can be used to wake up a thread idling in select() or
 * poll().
 */
class EventPipe {
  FileDescriptor r;

#ifndef HAVE_EVENTFD
  FileDescriptor w;
#endif

public:
  bool IsDefined() const {
    return r.IsDefined();
  }

  /**
   * Create the pipe.
   *
   * @return false on error
   */
  bool Create();

  /**
   * Returns the file descriptor that should be polled on.
   */
  int GetReadFD() const {
    return r.Get();
  }

  /**
   * Send a wakeup signal to the reader.
   */
  void Signal();

  /**
   * Read from the file descriptor.
   *
   * @return true if a signal was received
   */
  bool Read();
};

#endif
