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

#ifdef GNAV

#include "Units.hpp"
#include "Math/FastMath.h"
#include "DataField/Base.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "PeriodClock.hpp"
#include "Components.hpp"
#include "Hardware/AltairControl.hpp"

static WndForm *wf=NULL;

static void OnCloseClicked(WindowControl * Sender){
(void)Sender;
	wf->SetModalResult(mrOK);
}

static bool EnableAutoBrightness = true;
static int BrightnessValue = 0;

static void UpdateValues() {
  static PeriodClock last_time;
  if (!last_time.check_update(200))
    return;

  if (EnableAutoBrightness) {
    altair_control.SetBacklight(-100);
  } else {
    altair_control.SetBacklight(BrightnessValue);
  }
}

static void OnAutoData(DataField *Sender, DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daChange:
      EnableAutoBrightness = Sender->GetAsBoolean();
      UpdateValues();
    break;
  }
}


static void OnBrightnessData(DataField *Sender,
			     DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daChange:
      BrightnessValue = Sender->GetAsInteger();
      UpdateValues();
    break;
  }
}


static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnAutoData),
  DeclareCallBackEntry(OnBrightnessData),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};



void dlgBrightnessShowModal(void){
  wf = LoadDialog(CallBackTable,
		      XCSoarInterface::main_window,
		      _T("IDR_XML_BRIGHTNESS"));
  if (wf == NULL)
    return;

  WndProperty* wp;

  wp = (WndProperty*)wf->FindByName(_T("prpBrightness"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(BrightnessValue);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpAuto"));
  if (wp) {
    wp->GetDataField()->SetAsBoolean(EnableAutoBrightness);
    wp->RefreshDisplay();
  }
  wf->ShowModal();

  UpdateValues();

  delete wf;
}

#else /* !GNAV */

#include "Dialogs/Message.hpp"

void
dlgBrightnessShowModal()
{
  /* XXX this is ugly, non-Altair platforms should not even see the
     according menu item; not translating this superfluous message */
  MessageBoxX(_T("Only available on Altair"), _T("Brightness"),
              MB_OK|MB_ICONERROR);
}

#endif /* !GNAV */
