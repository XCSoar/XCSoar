// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FlarmDevice;

/**
 * Show the radio range statistics of a PowerFLARM (PFLAN) as a polar
 * plot, and offer to reset them.
 */
void
FlarmRangeStatisticsDialog(FlarmDevice &device);
