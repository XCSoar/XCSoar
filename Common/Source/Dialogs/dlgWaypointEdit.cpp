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
#include "InfoBoxLayout.h"
#include "Waypointparser.h"
#include "Math/FastMath.h"
#include "DataField/Enum.hpp"
#include "MainWindow.hpp"
#include "Compatibility/string.h"
#include "Components.hpp"
#include "Waypoint/Waypoint.hpp"

static WndForm *wf=NULL;
static Waypoint *global_wpt=NULL;

static WndButton *buttonName = NULL;
static WndButton *buttonComment = NULL;

static void UpdateButtons(void) {
  TCHAR text[MAX_PATH];
  if (buttonName) {
    if (!global_wpt->Name.size()) {
      _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Name")),
                gettext(TEXT("(blank)")));
    } else {
      _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Name")),
                global_wpt->Name.c_str());
    }
    buttonName->SetCaption(text);
  }
  if (buttonComment) {
    if (!global_wpt->Comment.size()) {
      _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Comment")),
                gettext(TEXT("(blank)")));
    } else {
      _stprintf(text,TEXT("%s: %s"), gettext(TEXT("Comment")),
                global_wpt->Comment.c_str());
    }
    buttonComment->SetCaption(text);
  }
}


static void OnNameClicked(WindowControl *Sender) {
	(void)Sender;
  if (buttonName) {
    TCHAR buff[NAME_SIZE+1];
    _stprintf(buff, TEXT("%s"), global_wpt->Name.c_str());
    dlgTextEntryShowModal(buff, NAME_SIZE);
    global_wpt->Name = buff;
  }
  UpdateButtons();
}


static void OnCommentClicked(WindowControl *Sender) {
	(void)Sender;
  if (buttonComment) {
    TCHAR buff[COMMENT_SIZE+1];
    _stprintf(buff, TEXT("%s"), global_wpt->Comment.c_str());
    dlgTextEntryShowModal(buff, COMMENT_SIZE);
    global_wpt->Comment = buff;
  }
  UpdateButtons();
}

static void SetUnits(void) {
  WndProperty* wp;
  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      wp->SetVisible(false);
    }
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    // hide this field for DD.dddd format
    if (wp) {
      wp->SetVisible(false);
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      wp->SetVisible(false);
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

  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeSign"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText((TEXT("W")));
    dfe->addEnumText((TEXT("E")));
    dfe->Set(sign);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }

  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(mm);
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(ss);
      wp->RefreshDisplay();
    }
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(mm);
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(1000.0*ss/60.0);
      wp->RefreshDisplay();
    }
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(10000.0*(mm+ss/60.0)/60.0);
      wp->RefreshDisplay();
    }
    break;
  }

  Units::LatitudeToDMS(global_wpt->Location.Latitude,
		       &dd, &mm, &ss, &sign);

  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeSign"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText((TEXT("S")));
    dfe->addEnumText((TEXT("N")));
    dfe->Set(sign);
    wp->RefreshDisplay();
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }

  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(mm);
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(ss);
      wp->RefreshDisplay();
    }
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(mm);
      wp->RefreshDisplay();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(1000.0*ss/60.0);
      wp->RefreshDisplay();
    }
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(10000.0*(mm+ss/60.0)/60.0);
      wp->RefreshDisplay();
    }
    break;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(
				   iround(global_wpt->Altitude
					  *ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlags"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Turnpoint"));
    dfe->addEnumText(TEXT("Airport"));
    dfe->addEnumText(TEXT("Landpoint"));

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

  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeSign"));
  if (wp) {
    sign = (wp->GetDataField()->GetAsInteger()==1);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
  if (wp) {
    dd = wp->GetDataField()->GetAsInteger();
  }

  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
    if (wp) {
      ss = wp->GetDataField()->GetAsInteger();
    }
    num = dd+mm/60.0+ss/3600.0;
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudemmm"));
    if (wp) {
      ss = wp->GetDataField()->GetAsInteger();
    }
    num = dd+(mm+ss/1000.0)/60.0;
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeDDDD"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    num = dd+mm/10000;
    break;
  }
  if (!sign) {
    num = -num;
  }

  global_wpt->Location.Longitude = num;

  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeSign"));
  if (wp) {
    sign = (wp->GetDataField()->GetAsInteger()==1);
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
  if (wp) {
    dd = wp->GetDataField()->GetAsInteger();
  }

  switch (Units::CoordinateFormat) {
  case 0: // ("DDMMSS");
  case 1: // ("DDMMSS.ss");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
    if (wp) {
      ss = wp->GetDataField()->GetAsInteger();
    }
    num = dd+mm/60.0+ss/3600.0;
    break;
  case 2: // ("DDMM.mmm");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudemmm"));
    if (wp) {
      ss = wp->GetDataField()->GetAsInteger();
    }
    num = dd+(mm+ss/1000.0)/60.0;
    break;
  case 3: // ("DD.dddd");
    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeDDDD"));
    if (wp) {
      mm = wp->GetDataField()->GetAsInteger();
    }
    num = dd+mm/10000;
    break;
  }
  if (!sign) {
    num = -num;
  }

  global_wpt->Location.Latitude = num;

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
  if (wp) {
    ss = wp->GetDataField()->GetAsInteger();
    if (ss==0) {
      WaypointAltitudeFromTerrain(*global_wpt, terrain);
    } else {
      global_wpt->Altitude = ss/ALTITUDEMODIFY;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFlags"));
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


static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgWaypointEditShowModal(Waypoint &way_point)
{
  global_wpt = &way_point;

  if (!InfoBoxLayout::landscape) {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWaypointEdit_L.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_WAYPOINTEDIT_L"));
  } else {
    wf = dlgLoadFromXML(CallBackTable,
                        TEXT("dlgWaypointEdit.xml"),
                        XCSoarInterface::main_window,
                        TEXT("IDR_XML_WAYPOINTEDIT"));
  }

  if (wf == NULL)
    return;

  buttonName = ((WndButton *)wf->FindByName(TEXT("cmdName")));
  if (buttonName) {
    buttonName->SetOnClickNotify(OnNameClicked);
  }

  buttonComment = ((WndButton *)wf->FindByName(TEXT("cmdComment")));
  if (buttonComment) {
    buttonComment->SetOnClickNotify(OnCommentClicked);
  }

  UpdateButtons();

  SetUnits();

  SetValues();

  wf->SetModalResult(mrCancel);

  if (wf->ShowModal()==mrOK) {
    GetValues();
  }

  delete wf;
}
