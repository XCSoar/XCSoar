// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * This is a stub implementation of NetComponents, 
 * used in builds without network support 
 * or for testing purposes.
 */

#include "NetComponents.hpp"

NetComponents::NetComponents(EventLoop &, CurlGlobal &,
                             const TrackingSettings &) noexcept
{
}

NetComponents::~NetComponents() noexcept = default;
