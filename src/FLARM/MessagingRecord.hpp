// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RadioFrequency.hpp"
#include "Id.hpp"
#include "util/StaticString.hxx"
#include <string>
#include <cstdint>

struct MessagingRecord {
  FlarmId id;
  std::string pilot;
  std::string plane_type;
  std::string registration;
  std::string callsign;
  RadioFrequency frequency = RadioFrequency::Null();
};