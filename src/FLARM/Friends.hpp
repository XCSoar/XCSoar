// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Color.hpp"

class FlarmId;

namespace FlarmFriends {

[[gnu::pure]]
FlarmColor
GetFriendColor(FlarmId id) noexcept;

void
SetFriendColor(FlarmId id, FlarmColor color) noexcept;

} // namespace FlarmFriends
