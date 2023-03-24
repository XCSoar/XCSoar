// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace Nook {

/**
 * initialize USB mode in Nook (must be rooted with USB Kernel)
 */
void InitUsb();

/**
 * Enter FastMode to eliminate full refresh of screen
 * requires Nook kernel rooted to support FastMode
 *
 * @return false if the operation has failed, e.g. because the
 * kernel does not support the mode
 */
bool EnterFastMode();

/**
 * Exit FastMode to restore full (slow) refresh of screen
 * requires Nook kernel rooted to support FastMode
 */
void ExitFastMode();

/**
 * Set Nook regulator's charge rate to 500mA.
 */
void SetCharge500();

/**
 * Set Nook regulator's charge rate to 100mA.
 */
void SetCharge100();

} // namespace Nook
