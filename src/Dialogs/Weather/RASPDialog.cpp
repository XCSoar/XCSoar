/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WeatherDialogs.hpp"
#include "Dialogs/HelpDialog.hpp"
#include "Dialogs/XML.hpp"
#include "Units/Units.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Screen/Layout.hpp"
#include "Form/Form.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Components.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

void
dlgWeatherShowModal()
{
  WndForm *wf = LoadDialog(nullptr, UIGlobals::GetMainWindow(),
                           _T("IDR_XML_WEATHER"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  assert(wp != nullptr);
  DataFieldEnum *dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->addEnumText(_("Now"));
  for (unsigned i = 1; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
    if (RASP.isWeatherAvailable(i)) {
      TCHAR timetext[10];
      _stprintf(timetext, _T("%04d"), RASP.IndexToTime(i));
      dfe->addEnumText(timetext, i);
    }
  }

  dfe->Set(RASP.GetTime());
  wp->RefreshDisplay();

  wp = (WndProperty *)wf->FindByName(_T("prpDisplayItem"));
  assert(wp != nullptr);
  dfe = (DataFieldEnum *)wp->GetDataField();
  dfe->EnableItemHelp(true);
  dfe->addEnumText(_("Terrain"));

  for (int i = 1; i <= 15; i++) {
    const TCHAR *label = RASP.ItemLabel(i);
    if (label != NULL)
      dfe->AddChoice(i, label, nullptr, RASP.ItemHelp(i));
  }
  dfe->Set(RASP.GetParameter());
  wp->RefreshDisplay();

  wf->ShowModal();

  wp = (WndProperty *)wf->FindByName(_T("prpTime"));
  assert(wp != nullptr);
  dfe = (DataFieldEnum *)wp->GetDataField();
  RASP.SetTime(dfe->GetValue());

  wp = (WndProperty *)wf->FindByName(_T("prpDisplayItem"));
  assert(wp != nullptr);
  dfe = (DataFieldEnum *)wp->GetDataField();
  RASP.SetParameter(dfe->GetValue());

  delete wf;
}

/*
  Todo:
  - units conversion in routine
  - load on demand
  - time based search
  - fix dialog
  - put label in map window as to what is displayed if not terrain
      (next to AUX)
  - Draw a legend on screen?
  - Auto-advance time index of forecast if before current time
*/
