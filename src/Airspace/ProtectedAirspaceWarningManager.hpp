// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Ptr.hpp"
#include "thread/Guard.hpp"

#include <optional>

class AirspaceWarning;
class AirspaceWarningManager;
class FlatProjection;

class ProtectedAirspaceWarningManager : public Guard<AirspaceWarningManager> {
public:
  explicit ProtectedAirspaceWarningManager(AirspaceWarningManager &awm) noexcept
    :Guard<AirspaceWarningManager>(awm) {}

  [[gnu::pure]]
  const FlatProjection &GetProjection() const noexcept;

  void Clear() noexcept;
  void AcknowledgeAll() noexcept;

  [[gnu::pure]]
  bool GetAckDay(const AbstractAirspace &airspace) const noexcept;

  void AcknowledgeDay(ConstAirspacePtr airspace, bool set=true) noexcept;
  void AcknowledgeWarning(ConstAirspacePtr airspace, bool set=true) noexcept;
  void AcknowledgeInside(ConstAirspacePtr airspace, bool set=true) noexcept;
  void Acknowledge(ConstAirspacePtr airspace) noexcept;

  [[gnu::pure]]
  bool IsEmpty() const noexcept;

  /**
   * Returns a copy of the highest priority warning, or an empty
   * instance if there is no warning.
   */
  [[gnu::pure]]
  std::optional<AirspaceWarning> GetTopWarning() const noexcept;
};
