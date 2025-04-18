// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ContestDijkstra.hpp"

/**
 * A quadrilateral task for the DMSt (Deutsche Meisterschaft im
 * Streckensegelflug).
 *
 * @see http://www.daec.de/fileadmin/user_upload/files/2012/sportarten/segelflug/sport/dmst/DMSt-WO_2012.pdf
 */
class DMStQuad : public ContestDijkstra {
public:
  explicit DMStQuad(const Trace &_trace) noexcept;
};
