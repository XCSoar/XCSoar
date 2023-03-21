// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Color.hpp"

class FlarmId;

namespace FlarmFriends
{
  FlarmColor GetFriendColor(FlarmId id);
  void SetFriendColor(FlarmId id, FlarmColor color);
};
