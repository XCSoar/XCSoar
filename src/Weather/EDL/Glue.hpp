// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"

namespace EDL {

/**
 * Keeps the shared EDL forecast hour aligned with GPS time while an
 * overlay is being maintained.
 */
class Glue final : public NullBlackboardListener {
public:
  void OnGPSUpdate(const MoreData &basic) override;
};

} // namespace EDL
