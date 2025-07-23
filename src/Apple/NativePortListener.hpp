// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class PortListener;

namespace NativePortListener {

void Initialise();
void Deinitialise();

/**
 * Returns a pointer to the provided listener.
 * Does not take ownership.
 */
PortListener *Create(PortListener &listener);

} // namespace NativePortListener
