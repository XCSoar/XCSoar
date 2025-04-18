// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "time/BrokenDateTime.hpp"
#include "util/StaticString.hxx"

struct TAF
{
  typedef StaticString<1024> ContentString;

  BrokenDateTime last_update;
  ContentString content;

  void clear() {
    last_update.year = 0;
    last_update.month = 0;
    last_update.day = 0;
    last_update.day_of_week = 0;
    last_update.hour = 0;
    last_update.minute = 0;
    last_update.second = 0;
    content.clear();
  }
};
