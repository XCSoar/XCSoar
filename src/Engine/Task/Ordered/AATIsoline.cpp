// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AATIsoline.hpp"
#include "Points/AATPoint.hpp"

AATIsoline::AATIsoline(const AATPoint &ap,
                       const FlatProjection &projection) noexcept
  :ell(ap.GetPrevious()->GetLocationRemaining(),
       ap.GetNext()->GetLocationRemaining(),
       ap.GetTargetLocation(), projection) {}
