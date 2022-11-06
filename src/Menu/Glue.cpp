/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "Glue.hpp"
#include "ButtonLabel.hpp"
#include "MenuBar.hpp"
#include "MenuData.hpp"
#include "util/Macros.hpp"

namespace MenuGlue {

void
SetLabelText(MenuBar &bar, unsigned index,
             const TCHAR *text, unsigned event) noexcept
{
  TCHAR buffer[100];
  const auto expanded = ButtonLabel::Expand(text, buffer, ARRAY_SIZE(buffer));
  if (expanded.visible)
    bar.ShowButton(index, expanded.enabled, expanded.text, event);
  else
    bar.HideButton(index);
}

void
Set(MenuBar &bar, const Menu &menu, const Menu *overlay, bool full) noexcept
{
  for (unsigned i = 0; i < menu.MAX_ITEMS; ++i) {
    const MenuItem &item = overlay != nullptr && (*overlay)[i].IsDefined()
      ? (*overlay)[i]
      : menu[i];

    if (full || item.IsDynamic())
      SetLabelText(bar, i, item.label, item.event);
  }
}

} // namespace MenuGlue
