// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class DataHandler;

namespace NativeInputListener {

void Initialise();
void Deinitialise();

/**
 * Create a Java NativeInputListener instance.  It is not bound to a
 * handler yet; call Set() to do this.
 */
DataHandler *Create(DataHandler &handler);

} // namespace NativeInputListener
