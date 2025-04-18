// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct NMEAInfo;

class AbstractReplay 
{
public:
  virtual ~AbstractReplay() {}

  virtual bool Update(NMEAInfo &data) = 0;
};
