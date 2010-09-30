/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Dialogs/Internal.hpp"
#include "Units.hpp"
#include "LocalTime.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Screen/Layout.hpp"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Components.hpp"

#include <stdio.h>

static WndForm *wf = NULL;

static void
OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static void
RASPGetTime(DataFieldEnum *Sender)
{
  int index = 0;
  for (unsigned i = 0; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
    if (RASP.isWeatherAvailable(i)) {
      if (RASP.GetTime() == i)
        Sender->Set(index);

      index++;
    }
  }
}

static void
RASPSetTime(DataFieldEnum *Sender)
{
  int index = 0;
  if (Sender->GetAsInteger() <= 0) {
    RASP.SetTime(0);
    return;
  }
  for (unsigned i = 0; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
    if (RASP.isWeatherAvailable(i)) {
      if (index == Sender->GetAsInteger())
        RASP.SetTime(i);

      index++;
    }
  }
}

static void
OnWeatherHelp(WindowControl * Sender)
{
  WndProperty *wp = (WndProperty*)Sender;
  int type = wp->GetDataField()->GetAsInteger();
  TCHAR caption[256];
  _tcscpy(caption, _("Weather parameters"));
  const TCHAR *label = RASP.ItemLabel(type);
  if (label != NULL) {
    _tcscat(caption, _T(": "));
    _tcscat(caption, label);
  }

  const TCHAR *help = RASP.ItemHelp(type);
  if (help == NULL)
    help = _("No help available on this item");

  dlgHelpShowModal(XCSoarInterface::main_window, caption, help);
}

static CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(OnWeatherHelp),
  DeclareCallBackEntry(NULL)
};

void
dlgWeatherShowModal(void)
{

  wf = LoadDialog(CallBackTable, XCSoarInterface::main_window,
                      !Layout::landscape ?
                      _T("IDR_XML_WEATHER_L") : _T("IDR_XML_WEATHER"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_T("Now"));
    for (unsigned i = 1; i < RasterWeather::MAX_WEATHER_TIMES; i++) {
      if (RASP.isWeatherAvailable(i)) {
        TCHAR timetext[10];
        _stprintf(timetext, _T("%04d"), RASP.IndexToTime(i));
        dfe->addEnumText(timetext);
      }
    }

    RASPGetTime(dfe);

    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayItem"));
  DataFieldEnum* dfe;
  if (wp) {
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(_("Terrain"));

    for (int i = 1; i <= 15; i++) {
      const TCHAR *label = RASP.ItemLabel(i);
      if (label != NULL)
        dfe->addEnumText(label);
    }
    dfe->Set(RASP.GetParameter());
    wp->RefreshDisplay();
  }

  wf->ShowModal();

  wp = (WndProperty*)wf->FindByName(_T("prpTime"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    RASPSetTime(dfe);
  }

  wp = (WndProperty*)wf->FindByName(_T("prpDisplayItem"));
  if (wp)
    RASP.SetParameter(wp->GetDataField()->GetAsInteger());

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
