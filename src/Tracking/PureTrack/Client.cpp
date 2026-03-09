// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Settings.hpp"

#include "co/Task.hxx"

namespace PureTrack {

Co::Task<void>
Client::Insert([[maybe_unused]] const Settings &settings,
               [[maybe_unused]] const Sample &sample)
{
  co_return;
}

} // namespace PureTrack
