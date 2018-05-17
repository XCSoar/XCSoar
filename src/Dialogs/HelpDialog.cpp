/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "HelpDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

#include <assert.h>

void
HelpDialog(const TCHAR *Caption, const TCHAR *HelpText)
{
  assert(HelpText != nullptr);

  const TCHAR *prefix = _("Help");

  StaticString<100> full_caption;
  if (Caption != nullptr) {
    full_caption.Format(_T("%s: %s"), prefix, Caption);
    Caption = full_caption.c_str();
  } else
    Caption = prefix;

  const auto &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), Caption,
                    new LargeTextWidget(look, HelpText));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
