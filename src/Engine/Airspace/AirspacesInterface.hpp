// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Airspace.hpp"
#include "Geo/Flat/BoostFlatBoundingBox.hpp"

#include <boost/geometry/index/rtree.hpp>
#include <boost/range/iterator_range_core.hpp>

/**
 * Abstract class for interface to #Airspaces database.
 * Allows references to the #Airspaces to be substituted for a
 * facade protected class where locking is required.
 */
class AirspacesInterface {
  struct AirspaceIndexable {
    using result_type = FlatBoundingBox;

    result_type operator()(const Airspace &airspace) const {
      return airspace;
    }
  };

public:
  using AirspaceVector = std::vector<Airspace>; /**< Vector of airspaces (used internally) */

  /**
   * Type of KD-tree data structure for airspace container
   */
  using AirspaceTree =
    boost::geometry::index::rtree<Airspace, boost::geometry::index::rstar<16>,
                                  AirspaceIndexable> ;

  using const_iterator = AirspaceTree::const_query_iterator;
  using const_iterator_range = boost::iterator_range<const_iterator>;
};
