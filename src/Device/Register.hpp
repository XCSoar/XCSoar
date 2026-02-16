// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/** \file
 *
 * This library manages the list of device drivers.
 */

#pragma once

#include <tchar.h>

struct DeviceRegister;

[[gnu::const]]
const DeviceRegister *
GetDriverByIndex(unsigned i);

[[gnu::pure]]
const DeviceRegister *
FindDriverByName(const char *name);

/**
 * Find the driver with the specified name, and return its display
 * name.  If no such driver was found, the specified name is returned.
 */
[[gnu::pure]]
const char *
FindDriverDisplayName(const char *name);
