// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CurrentAirspacesWidget.hpp"
#include "AirspaceWarningMonitor.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Language/Language.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Widget/ButtonWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "util/StaticString.hxx"

#include <cassert>
#include <memory>

[[gnu::pure]]
static StaticString<256>
FormatCaption(const AbstractAirspace &airspace,
              CurrentAirspacesWidget::Role role) noexcept
{
  StaticString<256> caption;
  if (role == CurrentAirspacesWidget::Role::Suppressed)
    caption.Format("%s  (%s)", airspace.GetName(), _("inside"));
  else
    caption.Format("%s  %s", airspace.GetName(), _("cleared"));
  return caption;
}

static std::unique_ptr<Widget>
MakeRowWidget(ProtectedAirspaceWarningManager &manager,
              const CurrentAirspacesWidget::Entry &entry) noexcept
{
  const auto caption = FormatCaption(*entry.airspace, entry.role);
  ConstAirspacePtr airspace = entry.airspace;
  ProtectedAirspaceWarningManager *manager_ptr = &manager;
  return std::make_unique<ButtonWidget>(
    UIGlobals::GetDialogLook().button,
    caption,
    [airspace = std::move(airspace), manager_ptr]() {
      dlgAirspaceDetails(airspace, manager_ptr);
    });
}

static std::unique_ptr<Widget>
MakeInner(ProtectedAirspaceWarningManager &manager,
          std::span<const CurrentAirspacesWidget::Entry> entries) noexcept
{
  assert(!entries.empty());
  if (entries.size() == 1)
    return MakeRowWidget(manager, entries[0]);

  return std::make_unique<TwoWidgets>(MakeRowWidget(manager, entries[0]),
                                      MakeRowWidget(manager, entries[1]),
                                      true);
}

CurrentAirspacesWidget::CurrentAirspacesWidget(
    AirspaceWarningMonitor &_monitor,
    ProtectedAirspaceWarningManager &_manager,
    std::span<const Entry> _entries) noexcept
  :SolidWidget(MakeInner(_manager, _entries)),
   monitor(_monitor), manager(_manager),
   n_entries(_entries.size())
{
  assert(!_entries.empty());
  assert(_entries.size() <= entries.size());
  for (unsigned i = 0; i < n_entries; ++i)
    entries[i] = _entries[i];
}

CurrentAirspacesWidget::~CurrentAirspacesWidget() noexcept
{
  assert(monitor.current_widget == this);
  monitor.current_widget = nullptr;
}

bool
CurrentAirspacesWidget::Matches(std::span<const Entry> other) const noexcept
{
  if (other.size() != n_entries)
    return false;
  for (unsigned i = 0; i < n_entries; ++i)
    if (!(entries[i] == other[i]))
      return false;
  return true;
}

unsigned
CurrentAirspacesWidget::CollectEntries(
    const ProtectedAirspaceWarningManager &manager,
    std::array<Entry, 2> &out) noexcept
{
  /* Suppressed entries take precedence over cleared, and we have at
     most two rows. Collect suppressed first, then cleared. */

  Entry suppressed[2];
  unsigned n_suppressed = 0;
  Entry cleared[2];
  unsigned n_cleared = 0;

  const ProtectedAirspaceWarningManager::Lease lease(manager);
  for (auto it = lease->begin(), end = lease->end(); it != end; ++it) {
    const AirspaceWarning &w = *it;
    if (!w.IsInside())
      continue;
    if (w.HasExplicitAck())
      continue;

    if (w.IsCoveredByClearance()) {
      if (n_suppressed < 2)
        suppressed[n_suppressed++] = Entry{w.GetAirspacePtr(),
                                           Role::Suppressed};
    } else if (w.IsCleared()) {
      if (n_cleared < 2)
        cleared[n_cleared++] = Entry{w.GetAirspacePtr(), Role::Cleared};
    }
  }

  /* Trigger condition: at least one suppressed entry. */
  if (n_suppressed == 0)
    return 0;

  unsigned n = 0;
  for (unsigned i = 0; i < n_suppressed && n < 2; ++i)
    out[n++] = suppressed[i];
  for (unsigned i = 0; i < n_cleared && n < 2; ++i)
    out[n++] = cleared[i];

  return n;
}
