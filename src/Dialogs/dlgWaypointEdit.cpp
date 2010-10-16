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
#include "InputEvents.h"
#include "Screen/Layout.hpp"
#include "WayPointFile.hpp"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "Components.hpp"
#include "Waypoint/Waypoint.hpp"
#include "StringUtil.hpp"
#include "Compiler.h"

#if defined(__BORLANDC__)  // due to compiler bug
  #include "RasterTerrain.h"
#endif

static WndForm *wf=NULL;
static Waypoint *global_wpt=NULL;

static WndButton *buttonName = NULL;
static WndButton *buttonComment = NULL;

static void UpdateButtons(void) {
  TCHAR text[MAX_PATH];
  if (buttonName) {
    if (!global_wpt->Name.size()) {
      _stprintf(text,_T("%s: %s"), _("Name"),
                _("(blank)"));
    } else {
      _stprintf(text,_T("%s: %s"), _("Name"),
                global_wpt->Name.c_str());
    }
    buttonName->SetCaption(text);
  }
  if (buttonComment) {
    if (!global_wpt->Comment.size()) {
      _stprintf(text,_T("%s: %s"), _("Comment"),
                _("(blank)"));
    } else {
      _stprintf(text,_T("%s: %s"), _("Comment"),
                global_wpt->Comment.c_str());
    }
    buttonComment->SetCaption(text);
  }
}


static void
OnNameClicked(gcc_unused WndButton &button)
{
  if (buttonName) {
    TCHAR buff[NAME_SIZE+1];
    _tcscpy(buff, global_wpt->Name.c_str());
    dlgTextEntryShowModal(buff, NAME_SIZE);
    global_wpt->Name = buff;
  }
  UpdateButtons();
}

static void
OnCommentClicked(gcc_unused WndButton &button)
{
  if (buttonComment) {
    TCHAR buff[COMMENT_SIZE+1];
    _tcscpy(buff, global_wpt->Comment.c_str());
    dlgTextEntryShowModal(buff, COMMENT_SIZE);
    global_wpt->Comment = buff;
  }
  UpdateButtons();
}

static void SetUnits(void) {
  WndProperty* wp;
  switch (Units::GetCoordinateFormat()) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudeDDDD"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudeDDDD"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudemmm"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudemmm"));
    if (wp) {
      wp->hide();
    }
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudeDDDD"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudeDDDD"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudeS"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudeS"));
    if (wp) {
      wp->hide();
    }
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudeM"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudeM"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudeS"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudeS"));
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLongitudemmm"));
    // hide this field for DD.dddd format
    if (wp) {
      wp->hide();
    }
    wp = (WndProperty*)wf->FindByName(_T("prpLatitudemmm"));
    if (wp) {
      wp->hide();
    }
    break;
  }
}

static void SetValues(void) {
  WndProperty* wp;
  bool sign;
  int dd,mm,ss;

  Units::LongitudeToDMS(global_wpt->Location.Longitude,
			&dd, &mm, &ss, &sign);

  wp = (WndProperty*)wf->FindByName(_T("prpLongitudeSign"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText((_T("W")));
    dfe->addEnumText((_T("E")));
    dfe->Set(sign);
    wp->RefreshDisplay();
  }

  LoadFormProperty(*wf, _T("prpLongitudeD"), dd);

  switch (Units::GetCoordinateFormat()) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    LoadFormProperty(*wf, _T("prpLongitudeM"), mm);
    LoadFormProperty(*wf, _T("prpLongitudeS"), ss);
    break;
  case 2: // ("DDMM.mmm");
    LoadFormProperty(*wf, _T("prpLongitudeM"), mm);
    LoadFormProperty(*wf, _T("prpLongitudemmm"), 1000 * fixed(ss) / 60);
    break;
  case 3: // ("DD.dddd");
    LoadFormProperty(*wf, _T("prpLongitudeDDDD"),
                     10000 * (fixed)(mm + ss) / 3600);
    break;
  }

  Units::LatitudeToDMS(global_wpt->Location.Latitude,
		       &dd, &mm, &ss, &sign);

  LoadFormProperty(*wf, _T("prpLatitudeD"), dd);

  wp = (WndProperty*)wf->FindByName(_T("prpLatitudeSign"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText((_T("S")));
    dfe->addEnumText((_T("N")));
    dfe->Set(sign);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(_T("prpLatitudeD"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(dd);
    wp->RefreshDisplay();
  }

  switch (Units::GetCoordinateFormat()) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    LoadFormProperty(*wf, _T("prpLatitudeM"), mm);
    LoadFormProperty(*wf, _T("prpLatitudeS"), ss);
    break;
  case 2: // ("DDMM.mmm");
    LoadFormProperty(*wf, _T("prpLatitudeM"), mm);
    LoadFormProperty(*wf, _T("prpLatitudemmm"), 1000 * fixed(ss) / 60);
    break;
  case 3: // ("DD.dddd");
    LoadFormProperty(*wf, _T("prpLatitudeDDDD"),
                     10000 * (fixed)(mm + ss) / 3600);
    break;
  }

  wp = (WndProperty*)wf->FindByName(_T("prpAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(iround(
        Units::ToUserUnit(global_wpt->Altitude, Units::AltitudeUnit)));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(_T("prpFlags"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();

    dfe->addEnumText(_T("Turnpoint"));
    dfe->addEnumText(_T("Airport"));
    dfe->addEnumText(_T("Landpoint"));

    if (global_wpt->Flags.Airport) {
      dfe->Set(1);
    } else if (global_wpt->Flags.LandPoint) {
      dfe->Set(2);
    } else {
      dfe->Set(0);
    }

    wp->RefreshDisplay();
  }
}


static void GetValues(void) {
  WndProperty* wp;
  bool sign = false;
  int dd = 0;
  double num=0, mm = 0, ss = 0; // mm,ss are numerators (division) so don't want to lose decimals

  sign = GetFormValueInteger(*wf, _T("prpLongitudeSign")) == 1;
  dd = GetFormValueInteger(*wf, _T("prpLongitudeD"));

  switch (Units::GetCoordinateFormat()) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    mm = GetFormValueInteger(*wf, _T("prpLongitudeM"));
    ss = GetFormValueInteger(*wf, _T("prpLongitudeS"));
    num = dd+mm/60.0+ss/3600.0;
    break;
  case 2: // ("DDMM.mmm");
    mm = GetFormValueInteger(*wf, _T("prpLongitudeM"));
    ss = GetFormValueInteger(*wf, _T("prpLongitudemmm"));
    num = dd+(mm+ss/1000.0)/60.0;
    break;
  case 3: // ("DD.dddd");
    mm = GetFormValueInteger(*wf, _T("prpLongitudeDDDD"));
    num = dd+mm/10000;
    break;
  }
  if (!sign) {
    num = -num;
  }

  global_wpt->Location.Longitude = Angle::degrees(fixed(num));

  sign = GetFormValueInteger(*wf, _T("prpLatitudeSign")) == 1;
  dd = GetFormValueInteger(*wf, _T("prpLatitudeD"));

  switch (Units::GetCoordinateFormat()) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    mm = GetFormValueInteger(*wf, _T("prpLatitudeM"));
    ss = GetFormValueInteger(*wf, _T("prpLatitudeS"));
    num = dd+mm/60.0+ss/3600.0;
    break;
  case 2: // ("DDMM.mmm");
    mm = GetFormValueInteger(*wf, _T("prpLatitudeM"));
    ss = GetFormValueInteger(*wf, _T("prpLatitudemmm"));
    num = dd+(mm+ss/1000.0)/60.0;
    break;
  case 3: // ("DD.dddd");
    mm = GetFormValueInteger(*wf, _T("prpLatitudeDDDD"));
    num = dd+mm/10000;
    break;
  }
  if (!sign) {
    num = -num;
  }

  global_wpt->Location.Latitude = Angle::degrees(fixed(num));

  ss = GetFormValueInteger(*wf, _T("prpAltitude"));
  global_wpt->Altitude = ss == 0 && terrain != NULL
    ? fixed(WayPointFile::AltitudeFromTerrain(global_wpt->Location, *terrain))
    : Units::ToSysUnit(fixed(ss), Units::AltitudeUnit);

  wp = (WndProperty*)wf->FindByName(_T("prpFlags"));
  if (wp) {
    int myflag = wp->GetDataField()->GetAsInteger();
    switch(myflag) {
    case 1:
      global_wpt->Flags.TurnPoint = true;
      global_wpt->Flags.Airport = true;
      break;
    case 2:
      global_wpt->Flags.LandPoint = true;
      break;
    default:
      global_wpt->Flags.TurnPoint = true;
      global_wpt->Flags.Airport = false;
      global_wpt->Flags.LandPoint = false;
    };
  }
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

bool
dlgWaypointEditShowModal(Waypoint &way_point)
{
  global_wpt = &way_point;

  if (!Layout::landscape) {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_WAYPOINTEDIT_L"));
  } else {
    wf = LoadDialog(CallBackTable,
                        XCSoarInterface::main_window,
                        _T("IDR_XML_WAYPOINTEDIT"));
  }

  if (wf == NULL)
    return false;

  buttonName = ((WndButton *)wf->FindByName(_T("cmdName")));
  if (buttonName) {
    buttonName->SetOnClickNotify(OnNameClicked);
  }

  buttonComment = ((WndButton *)wf->FindByName(_T("cmdComment")));
  if (buttonComment) {
    buttonComment->SetOnClickNotify(OnCommentClicked);
  }

  UpdateButtons();

  SetUnits();

  SetValues();

  wf->SetModalResult(mrCancel);

  bool retval = false;
  if (wf->ShowModal()==mrOK) {
    GetValues();
    retval = true;
  }

  delete wf;
  return retval;
}
