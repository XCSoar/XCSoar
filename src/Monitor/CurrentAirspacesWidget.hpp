// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/SolidWidget.hpp"
#include "Engine/Airspace/Ptr.hpp"

#include <array>
#include <cstdint>
#include <span>

class AirspaceWarningMonitor;
class ProtectedAirspaceWarningManager;

/**
 * Bottom-area widget that lists airspaces the aircraft is currently
 * inside, for when the related INSIDE warning is suppressed by a clearance
 */
class CurrentAirspacesWidget final : public SolidWidget {
public:
  enum class Role : uint8_t {
    Suppressed,
    Cleared,
  };

  struct Entry {
    ConstAirspacePtr airspace;
    Role role;

    bool operator==(const Entry &other) const noexcept {
      return airspace.get() == other.airspace.get() && role == other.role;
    }
  };

  /**
   * Scan the warning manager and collect up to two entries to display.
   * Suppressed entries (covered by clearance) take precedence over
   * cleared entries. Returns 0 if there is no suppressed entry.
   */
  static unsigned CollectEntries(const ProtectedAirspaceWarningManager &manager,
                                 std::array<Entry, 2> &out) noexcept;

  CurrentAirspacesWidget(AirspaceWarningMonitor &_monitor,
                         ProtectedAirspaceWarningManager &_manager,
                         std::span<const Entry> _entries) noexcept;

  ~CurrentAirspacesWidget() noexcept override;

  /**
   * Does the widget already display exactly these entries? Returning
   * true means no recreation is needed.
   */
  [[gnu::pure]]
  bool Matches(std::span<const Entry> other) const noexcept;

private:
  AirspaceWarningMonitor &monitor;
  ProtectedAirspaceWarningManager &manager;

  std::array<Entry, 2> entries;
  unsigned n_entries;
};
