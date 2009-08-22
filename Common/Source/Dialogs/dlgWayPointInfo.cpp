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

#include "Statistics.h"
#include "externs.h"
#include "McReady.h"
#include "Dialogs/dlgTools.h"

static int page=0;


void NextPage(int Step){
  page += Step;
  if (page > 3)
    page = 0;
  if (page < 0)
    page = 3;
  Update();
}

void OnNextClicked(WindowControl * Sender){
  NextPage(+1);
}

void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){

  if (wGrid->GetFocused())
    return(0);

  switch(wParam & 0xffff){
    case VK_LEFT:
      NextPage(-1);
    return(0);
    case VK_RIGHT:
      NextPage(+1);
    return(0);
  }
  return(1);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAnalysisPaint),
  DeclareCallBackEntry(OnNextClicked),
  DeclareCallBackEntry(OnPrevClicked),
  DeclareCallBackEntry(NULL)
};

void dlgAnalysisShowModal(void){

  wf = dlgLoadFromXML(CallBackTable, "T:\\Project\\WINCE\\TNAV\\XCSoar\\dlgWayPointInfo.xml", hWndMainWindow);

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  Update();

  wf->ShowModal();

  delete wf;

  wf = NULL;

}
