/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "StdAfx.h"
#include "XCSoar.h"
#include "externs.h"
#include "Units.h"
#include "InputEvents.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "Utils.h"

static WndForm *wf=NULL;


//
//

static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}




static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};



void dlgHelpShowModal(const TCHAR* Caption, const TCHAR* HelpText) {
  if (!Caption || !HelpText) {
    return;
  }

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgHelp_L.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_HELP_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgHelp.xml"),
                        hWndMainWindow,
                        TEXT("IDR_XML_HELP"));
  }
  WndProperty* wp;

  if (wf) {

    TCHAR fullcaption[100];
    _stprintf(fullcaption,TEXT("Help: %s"), Caption);

    wf->SetCaption(fullcaption);

    wp = (WndProperty*)wf->FindByName(TEXT("prpHelpText"));
    if (wp) {
      wp->SetText(HelpText);
      wp->RefreshDisplay();
    }
    wf->ShowModal();
    delete wf;
  }
  wf = NULL;

}


