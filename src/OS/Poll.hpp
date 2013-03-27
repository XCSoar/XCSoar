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

#ifndef XCSOAR_POLL_HPP
#define XCSOAR_POLL_HPP

#include "Compiler.h"

#include <vector>
#include <algorithm>

#include <poll.h>

/**
 * This class can poll for events on many file descriptors.  It is not
 * thread safe.
 */
class Poll {
  typedef std::vector<struct pollfd> List;

  List list;

public:
  /**
   * Mask bit for "file is ready for reading".
   */
  static constexpr unsigned READ = POLLIN;

  /**
   * Mask bit for "file is ready for writing".
   */
  static constexpr unsigned WRITE = POLLOUT;

protected:
  /**
   * Helper class for method Find().
   */
  struct CompareFD {
    int fd;

    CompareFD(int fd):fd(fd) {}

    bool operator()(const struct pollfd &p) const {
      return p.fd == fd;
    }
  };

  List::iterator Find(int fd) {
    return std::find_if(list.begin(), list.end(), CompareFD(fd));
  }

public:
  /**
   * Clear the file descriptor list.
   */
  void Clear() {
    list.clear();
  }

  /**
   * Register a file descriptor.
   *
   * @param mask the bit mask of interesting events; may be 0
   */
  void SetMask(int fd, unsigned mask);

  /**
   * Unregister a file descriptor.
   */
  void Remove(int fd);

  /**
   * Wait for an event on any of the file descriptors.
   *
   * @param timeout_ms a timeout in milliseconds; the method will
   * return successfully if the timeout has expired; -1 means no
   * timeout (the default)
   * @return false on error
   */
  bool Wait(int timeout_ms=-1) {
    return poll(&list[0], list.size(), timeout_ms) >= 0;
  }

  /**
   * This iterator class wraps the std::vector<pollfd> iterator.  It
   * skips items without an event, and returns only the integer file
   * descriptor.
   */
  class const_iterator {
    List::const_iterator i, end;

  public:
    const_iterator(List::const_iterator begin, List::const_iterator end)
      :i(begin), end(end) {
      FindResult();
    }

    bool operator==(const const_iterator &other) const {
      return i == other.i;
    }

    bool operator!=(const const_iterator &other) const {
      return i != other.i;
    }

    const_iterator &operator++() {
      ++i;
      FindResult();
      return *this;
    }

    const_iterator operator++(int) {
      const const_iterator result = *this;
      ++i;
      FindResult();
      return result;
    }

    int operator*() const {
      return i->fd;
    }

    unsigned GetMask() const {
      return i->revents;
    }

  protected:
    void FindResult() {
      while (i != end && i->revents == 0)
        ++i;
    }
  };

  gcc_pure
  const_iterator begin() const {
    return const_iterator(list.begin(), list.end());
  }

  gcc_pure
  const_iterator end() const {
    return const_iterator(list.end(), list.end());
  }
};

#endif
