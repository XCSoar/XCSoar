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

#include "InfoBoxes/Content/Radio.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

void
UpdateInfoBoxFrequency(InfoBoxData & data, const RadioFrequency & freq, const TCHAR * freq_name)
{
  if(freq.IsDefined()) {
    TCHAR buffer[32];
    freq.Format(buffer, 32);
    data.SetValue(buffer);
    data.SetComment(freq_name);
  }
  else {
    data.SetValueInvalid();
    data.SetCommentInvalid();
  }
}

void
UpdateInfoBoxActiveFrequency(InfoBoxData &data)
{
  const auto &settings_radio =
    CommonInterface::GetComputerSettings().radio;
  UpdateInfoBoxFrequency(data, settings_radio.active_frequency, settings_radio.active_name);
}

void
UpdateInfoBoxStandbyFrequency(InfoBoxData &data)
{
  const auto &settings_radio =
    CommonInterface::GetComputerSettings().radio;
  UpdateInfoBoxFrequency(data, settings_radio.standby_frequency, settings_radio.standby_name);
}

