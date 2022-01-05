/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}

*/

#include "AirspaceEnterMonitor.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Input/InputQueue.hpp"

/**
 * Does the "current" set contain at least one item that is not
 * present in the "previous" set?
 */
template<typename T>
[[gnu::pure]]
static bool
ContainsNewItem(const std::set<T> &current,
                const std::set<T> &previous) noexcept
{
  const auto key_comp = current.key_comp();

  for (auto c = current.begin(), p = previous.begin();
       c != current.end(); ++c, ++p) {
    while (true) {
      if (p == previous.end() || key_comp(*c, *p))
        /* this item does not exist in the "previous" set */
        return true;

      if (!key_comp(*p, *c))
        /* both are equal: found it */
        break;

      /* this "previous" item is no longer present in "current": skip
         it */
      ++p;
    }
  }

  /* found matches for all "current" items in "previous" */
  return false;
}

[[gnu::pure]]
static std::set<ConstAirspacePtr>
CollectNearAirspaces(const AirspaceWarningManager &warnings) noexcept
{
  std::set<ConstAirspacePtr> result;

  for (const auto &i : warnings)
    if (i.IsActive())
      result.emplace(i.GetAirspacePtr());

  return result;
}

[[gnu::pure]]
static std::set<ConstAirspacePtr>
CollectInsideAirspaces(const AirspaceWarningManager &warnings) noexcept
{
  std::set<ConstAirspacePtr> result;

  for (const auto &i : warnings)
    if (i.IsInside() && i.IsActive())
      result.emplace(i.GetAirspacePtr());

  return result;
}

inline void
AirspaceEnterMonitor::Update(const AirspaceWarningManager &warnings) noexcept
{
  const auto serial = warnings.GetSerial();
  if (serial == last_serial)
    /* no change */
    return;

  auto near_ = CollectNearAirspaces(warnings);
  if (ContainsNewItem(near_, last_near))
    InputEvents::processGlideComputer(GCE_AIRSPACE_NEAR);

  auto inside = CollectInsideAirspaces(warnings);
  if (ContainsNewItem(inside, last_inside))
    InputEvents::processGlideComputer(GCE_AIRSPACE_ENTER);

  last_serial = serial;
  last_near = std::move(near_);
  last_inside = std::move(inside);
}

void
AirspaceEnterMonitor::Update(const NMEAInfo &basic,
                             const DerivedInfo &calculated,
                             const ComputerSettings &settings) noexcept
{
  const ProtectedAirspaceWarningManager::Lease lease{protected_warnings};
  Update(lease);
}
