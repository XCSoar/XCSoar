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

class FlarmRangeComputer;

/**
 * Show the range estimate XCSoar collects from the traffic a Classic
 * FLARM reports, which has no statistics of its own, and offer to
 * reset it.
 */
void
FlarmRangeEstimateDialog(FlarmDevice &device, FlarmRangeComputer &computer);
