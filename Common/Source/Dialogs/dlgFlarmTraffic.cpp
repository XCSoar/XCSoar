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
#include "Screen/Layout.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "Blackboard.hpp"
#include "UtilsFLARM.hpp"
#include "Math/Earth.hpp"
#include "MainWindow.hpp"

#include <assert.h>

static int page=0;
static WndForm *wf=NULL;
static WndListFrame *wDetails=NULL;

static int GetActiveFlarmTrafficCount();

static void Update(){
  wDetails->SetLength(GetActiveFlarmTrafficCount());
  wDetails->invalidate();
}

static void
FlarmCursorCallback(unsigned i)
{
  const FLARM_TRAFFIC &traffic = XCSoarInterface::Basic().FLARM_Traffic[i];

  WndButton *set_cn_button = (WndButton *)wf->FindByName(_T("cmdSetCN"));
  WndButton *track_button = (WndButton *)wf->FindByName(_T("cmdTrack"));

  if (traffic.ID != 0) {
    if (LookupFLARMDetails(traffic.ID) == NULL) {
      // not existing en primary or secondary flarm id list
      set_cn_button->SetCaption(_T("Set CN"));
      set_cn_button->show();
    } else {
      // the id was found - is it from secondary list ?
      int index = LookupSecondaryFLARMId(traffic.ID);

      if (index != -1) {
        set_cn_button->SetCaption(_T("Edit CN"));
        set_cn_button->show();
      } else
        set_cn_button->hide();
    }

    track_button->show();
  } else {
    track_button->hide();
    set_cn_button->hide();
  }
}

static void
OnPaintDetailsListItem(Canvas &canvas, const RECT rc, unsigned i)
{
  if (i >= FLARM_MAX_TRAFFIC)
    return;

  TCHAR tmp[100];
  TCHAR text[100];

  double range;
  double bear;

  const FLARM_TRAFFIC &traffic = XCSoarInterface::Basic().FLARM_Traffic[i];
  if (traffic.ID == 0)
    return;

  DistanceBearing(XCSoarInterface::Basic().Location,
                  traffic.Location,
                  &range,
                  &bear);

  _stprintf(tmp, _T("%3s %3ld %+3.1f %5ld"),
            traffic.Name,
            (int)(SPEEDMODIFY * traffic.Speed),
#ifdef FLARM_AVERAGE
            LIFTMODIFY * traffic.Average30s,
#else
            0.0,
#endif
            (int)(ALTITUDEMODIFY * traffic.Altitude));
  _stprintf(text, _T("%s %3.0lf %2.1f"),
            tmp,
            bear,
            DISTANCEMODIFY * range);

  canvas.text(rc.left + Layout::FastScale(2), rc.top + Layout::FastScale(2),
              text);
}

static int GetActiveFlarmTrafficCount()
{
  int count = 0;
  for (int i=0; i<FLARM_MAX_TRAFFIC; i++)
    {
      if (XCSoarInterface::Basic().FLARM_Traffic[i].ID!=0)
	{
	  count++;
	}
    }
  return count;
}

static void SelectAsTeamTrack()
{
  int index = wDetails->GetCursorIndex();

  if (index < 0 || index >= FLARM_MAX_TRAFFIC)
    return;

  if (XCSoarInterface::Basic().FLARM_Traffic[index].Name[0] == 0) {
    XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[0] = 0;
  } else {
    // copy the 3 first chars from the name
    for (int z = 0; z < 3; z++)
      XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[z] = XCSoarInterface::Basic().FLARM_Traffic[index].Name[z];
    XCSoarInterface::SetSettingsComputer().TeamFlarmCNTarget[3] = 0;
  }

  // now tracking !
  XCSoarInterface::SetSettingsComputer().TeamFlarmIdTarget = XCSoarInterface::Basic().FLARM_Traffic[index].ID;
  XCSoarInterface::SetSettingsComputer().TeamFlarmTracking = true;
  XCSoarInterface::SetSettingsComputer().TeammateCodeValid = false;
}

static void OnTrackClicked(WindowControl * Sender)
{
  (void)Sender;
  SelectAsTeamTrack();
  wf->SetModalResult(mrOK);
}

static void OnSetCNClicked(WindowControl * Sender)
{
  (void)Sender;

  int index = wDetails->GetCursorIndex();

  if (index < 0 || index >= FLARM_MAX_TRAFFIC)
    return;

  TCHAR newName[21];
  newName[0] = 0;
  if(dlgTextEntryShowModal(newName, 4))
    AddFlarmLookupItem(XCSoarInterface::Basic().FLARM_Traffic[index].ID,
                       newName, true);
}


static void OnCloseClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(WindowControl *Sender, unsigned key_code)
{
  (void)Sender;

  switch(key_code) {
  case VK_LEFT:
  case '6':
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    //      NextPage(-1);
    //((WndButton *)wf->FindByName(_T("cmdPrev")))->SetFocused(true, NULL);
    return true;
  case VK_RIGHT:
  case '7':
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    //      NextPage(+1);
    //((WndButton *)wf->FindByName(_T("cmdNext")))->SetFocused(true, NULL);
    return true;

  default:
    return false;
  }
}

static int OnTimerNotify(WindowControl * Sender) {
  (void)Sender;
  Update();
  return 0;
}

static void OnListEnter(WindowControl * Sender,
			WndListFrame::ListInfo_t *ListInfo)
{
  SelectAsTeamTrack();
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnTrackClicked),
  DeclareCallBackEntry(OnSetCNClicked),
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};






void dlgFlarmTrafficShowModal(void){
  static bool first=true;
  if (first) {

    first=false;
  }

  if (Layout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgFlarmTraffic_L.xml"),
			XCSoarInterface::main_window,
			_T("IDR_XML_FLARMTRAFFIC_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        _T("dlgFlarmTraffic.xml"),
			XCSoarInterface::main_window,
			_T("IDR_XML_FLARMTRAFFIC"));
  }

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(_T("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wDetails = (WndListFrame*)wf->FindByName(_T("frmDetails"));
  wDetails->SetEnterCallback(OnListEnter);
  wDetails->SetCursorCallback(FlarmCursorCallback);
  wDetails->SetPaintItemCallback(OnPaintDetailsListItem);

  wDetails->SetBorderKind(BORDERLEFT);

  page = 0;

  Update();

  wf->SetTimerNotify(OnTimerNotify);

  wf->ShowModal();

  delete wf;

  wf = NULL;

}

