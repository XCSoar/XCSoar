// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class OperationEnvironment;

/**
 * Apply and propagate settings received from external devices.
 *
 * @return true if a setting was changed
 */
bool
ApplyExternalSettings(OperationEnvironment &env) noexcept;
