// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProtectedAirspaceWarningManager;
class AbstractAirspace;

/**
 * Match only airspaces that are "active", see
 * AirspaceWarningManager::IsActive() for a definition.
 *
 * The ProtectedAirspaceWarningManager attribute is optional.  It will
 * only query AbstractAirspace::IsActive() if the
 * ProtectedAirspaceWarningManager is nullptr.
 */
class ActiveAirspacePredicate {
  const ProtectedAirspaceWarningManager *warnings;

public:
  constexpr
  ActiveAirspacePredicate(const ProtectedAirspaceWarningManager *_warnings)
    :warnings(_warnings) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace &airspace) const;
};
