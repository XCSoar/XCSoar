// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/Glue.hpp"

/**
 * Darwin's static linker resolves weak undefined symbols from archives,
 * so Details.cpp's weak SaveFlarmMessagingPeriodic would pull Glue.o into
 * unit tests. Provide a strong no-op stub for tests that need FlarmDetails
 * without the full FLARM glue stack.
 */
void
SaveFlarmMessagingPeriodic() noexcept
{
}
