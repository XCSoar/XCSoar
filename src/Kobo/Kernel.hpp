// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once
/**
 * Install the given kernel image on /dev/mmcblk0.
 */
bool
KoboInstallKernel(const char *uimage_path);

/**
 * Is the kernel that is currently running a custom one?
 */
[[gnu::const]]
bool
IsKoboCustomKernel();

/**
 * Are we currently in OTG host mode?
 */
[[gnu::pure]]
bool
IsKoboOTGHostMode();
