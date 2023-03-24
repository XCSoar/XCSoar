// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class SearchPointVector;

/**
 * Convert the vector to a convex hull.  This may prune points.
 *
 * @param sps Input vector of points (may be unordered)
 *
 * @param sign_tolerance the tolerance for the direction sign; -1
 * for automatic tolerance
 *
 * @return changed Return status as to whether input vector was
 * altered (pruned) or not
 *
 * @author http://www.drdobbs.com/cpp/201806315?pgno=4
 */
bool
PruneInterior(SearchPointVector &v, double sign_tolerance = -1) noexcept;
