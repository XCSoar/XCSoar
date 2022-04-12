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

#include "Form/DataField/EnumListNumPadAdapter.hpp"
#include "Form/DataField/Base.hpp"
#include "Form/DataField/Enum.hpp"
#include "ui/event/KeyCode.hpp"

EnumListNumPadAdapter::EnumListNumPadAdapter(
    NumPadWidgetInterface *_numPadWidget) : NumPadAdapter(_numPadWidget)
{

}

void
EnumListNumPadAdapter::UpdateButtons() noexcept
{

}
void
EnumListNumPadAdapter::OnButton(unsigned buttonIndex) noexcept
{

}

void
EnumListNumPadAdapter::OnCursorMoved([[maybe_unused]] unsigned index) noexcept
{

  DataFieldEnum *df = dynamic_cast<DataFieldEnum*>(dataField);
  if (df != nullptr) {
    df->ModifyValue(index);
    numPad->SetCursorIndex(df->GetValue());
    if(refreshEditFieldFunction)
      refreshEditFieldFunction();
    /* A mouse click on the ListControl aquires focus.
    * but the NumPadAdapter should never have focus.
    * It get's keyboard events from WndProperty.
    */
    if(setFocusEditFieldFunction)
      setFocusEditFieldFunction();
  }
}

void
EnumListNumPadAdapter::OnModified() noexcept
{
  DataFieldEnum *df = dynamic_cast<DataFieldEnum*>(dataField);

  if (df != nullptr) {
  }

}

bool
EnumListNumPadAdapter::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_RIGHT:
  case KEY_LEFT:
    return true;
  }
  return false;
}

bool
EnumListNumPadAdapter::OnKeyDown(unsigned key_code) noexcept
{
  DataFieldEnum *df = dynamic_cast<DataFieldEnum*>(dataField);
  if (df != nullptr)
    switch (key_code) {
    case KEY_RIGHT:
      df->Inc();
      df->ModifyValue(df->GetAsDisplayString());

      numPad->SetCursorIndex(df->GetValue());
      df->ModifyValue(df->GetAsDisplayString());
      return true;
    case KEY_LEFT:
      df->Dec();
      numPad->SetCursorIndex(df->GetValue());
      return true;
    }
  return false;
}

