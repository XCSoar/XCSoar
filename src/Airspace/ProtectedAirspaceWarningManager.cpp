// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"

const FlatProjection &
ProtectedAirspaceWarningManager::GetProjection() const noexcept
{
  /* access to FlatProjection does not need to be protected */
  UnprotectedLease lease(const_cast<ProtectedAirspaceWarningManager &>(*this));
  return lease->GetProjection();
}

void
ProtectedAirspaceWarningManager::Clear() noexcept
{
  ExclusiveLease lease(*this);
  lease->clear();
}

void
ProtectedAirspaceWarningManager::AcknowledgeAll() noexcept
{
  ExclusiveLease lease(*this);
  lease->AcknowledgeAll();
}

bool
ProtectedAirspaceWarningManager::IsEmpty() const noexcept
{
  Lease lease(*this);
  return lease->empty();
}

bool
ProtectedAirspaceWarningManager::GetAckDay(const AbstractAirspace &airspace) const noexcept
{
  Lease lease(*this);
  return lease->GetAckDay(airspace);
}

void
ProtectedAirspaceWarningManager::AcknowledgeDay(ConstAirspacePtr airspace,
                                                const bool set) noexcept
{
  ExclusiveLease lease(*this);
  lease->AcknowledgeDay(std::move(airspace), set);
}

void
ProtectedAirspaceWarningManager::AcknowledgeWarning(ConstAirspacePtr airspace,
                                                    const bool set) noexcept
{
  ExclusiveLease lease(*this);
  lease->AcknowledgeWarning(std::move(airspace), set);
}

void
ProtectedAirspaceWarningManager::AcknowledgeInside(ConstAirspacePtr airspace,
                                                   const bool set) noexcept
{
  ExclusiveLease lease(*this);
  lease->AcknowledgeInside(std::move(airspace), set);
}

void
ProtectedAirspaceWarningManager::Acknowledge(ConstAirspacePtr airspace) noexcept
{
  ExclusiveLease lease(*this);
  lease->Acknowledge(std::move(airspace));
}

std::optional<AirspaceWarning>
ProtectedAirspaceWarningManager::GetTopWarning() const noexcept
{
  const Lease lease(*this);
  if (auto i = lease->begin(); i != lease->end())
    return *i;

  return std::nullopt;
}
