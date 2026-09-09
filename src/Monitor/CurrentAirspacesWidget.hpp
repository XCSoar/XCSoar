// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Engine/Airspace/Ptr.hpp"

#include <array>
#include <cstdint>
#include <span>

class AirspaceWarningMonitor;
class ProtectedAirspaceWarningManager;

/**
 * Bottom-area widget shown when the aircraft is inside an airspace
 * whose INSIDE warning is suppressed by a clearance. Lists up to two
 * rows: suppressed warnings first, then clearances with a "Revoke" button.
 */
class CurrentAirspacesWidget final : public WindowWidget {
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

  /**
   * Invoked by a row's "revoke" button: drop the clearance on the given
   * airspace. This might cause the object to have been destroyed by the time
   * this call returns; the caller must not touch it afterwards.
   */
  void RevokeClearance(ConstAirspacePtr airspace) noexcept;

  /* virtual methods from class Widget */
  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  AirspaceWarningMonitor &monitor;
  ProtectedAirspaceWarningManager &manager;

  std::array<Entry, 2> entries;
  unsigned n_entries;
};
