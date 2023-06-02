// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct InfoBoxLook;
class ContainerWindow;

namespace InfoBoxLayout { struct Layout; }

namespace InfoBoxManager
{

extern InfoBoxLayout::Layout layout;

void
ProcessTimer() noexcept;

void
SetDirty() noexcept;

void
ScheduleRedraw() noexcept;

void
Create(ContainerWindow &parent, const InfoBoxLayout::Layout &layout,
       const InfoBoxLook &look) noexcept;

void
Destroy() noexcept;

void
Show() noexcept;

void
Hide() noexcept;

/**
 * Opens a dialog to select the InfoBox contents for
 * the InfoBox indicated by id, or the focused InfoBox.
 * @param id The id of the InfoBox to configure.  If negative,
 * then it configures the focused InfoBox if there is one.
 */
void
ShowInfoBoxPicker(const int id = -1) noexcept;

} // namespace InfoBoxManager
