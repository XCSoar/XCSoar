// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BlackboardListener.hpp"

void
NullBlackboardListener::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
}

void
NullBlackboardListener::OnCalculatedUpdate([[maybe_unused]] const MoreData &basic,
                                           [[maybe_unused]] const DerivedInfo &calculated)
{
}

void
NullBlackboardListener::OnComputerSettingsUpdate([[maybe_unused]] const ComputerSettings &settings)
{
}

void
NullBlackboardListener::OnUISettingsUpdate([[maybe_unused]] const UISettings &settings)
{
}
