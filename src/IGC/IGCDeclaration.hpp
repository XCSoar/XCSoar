// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"
#include "Geo/GeoPoint.hpp"

struct IGCDeclarationHeader {
  /** Date and time of the declaration */
  BrokenDateTime datetime;

  /** Date of the flight */
  BrokenDate flight_date;

  /** Task number on the day */
  char task_id[4];

  /** Number of task turnpoints, excluding start and finish */
  unsigned num_turnpoints;

  /** Optional name of the task */
  StaticString<256> task_name;
};

struct IGCDeclarationTurnpoint {
  /** Location of the turnpoint */
  GeoPoint location;

  /** Optional name of the turnpoint */
  StaticString<256> name;
};
