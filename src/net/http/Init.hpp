// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

class EventLoop;
class CurlGlobal;

namespace Net {

#if defined(HAVE_HTTP)

extern CurlGlobal *curl;

/**
 * Global initialisation of the network library.
 */
void
Initialise(EventLoop &event_loop);

void Deinitialise();

#else

static inline void
Initialise(EventLoop &)
{
}

static inline void Deinitialise() {}

#endif

class ScopeInit {
public:
  ScopeInit(EventLoop &event_loop) {
    Initialise(event_loop);
  }

  ~ScopeInit() noexcept {
    Deinitialise();
  }

  ScopeInit(const ScopeInit &) = delete;
  ScopeInit &operator=(const ScopeInit &) = delete;
};

} // namespace Net
