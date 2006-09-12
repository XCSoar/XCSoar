/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "externs.h"
#include "units.h"
#include "device.h"
#include "InputEvents.h"
#include "WindowControls.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "Waypointparser.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;
static WAYPOINT *global_wpt=NULL;

static WndButton *buttonName = NULL;
static WndButton *buttonComment = NULL;

static void UpdateButtons(void) {
  TCHAR text[MAX_PATH];
  if (buttonName) {
    if (_tcslen(global_wpt->Name)<=0) {
      _stprintf(text,TEXT("Name: (blank)"));
    } else {
      _stprintf(text,TEXT("Name: %s"),global_wpt->Name);
    }
    buttonName->SetCaption(text);
  }
  if (buttonComment) {
    if (_tcslen(global_wpt->Comment)<=0) {
      _stprintf(text,TEXT("Comment: (blank)"));
    } else {
      _stprintf(text,TEXT("Comment: %s"),global_wpt->Comment);
    }
    buttonComment->SetCaption(text);
  }
}


static void OnNameClicked(WindowControl *Sender) {
  if (buttonName) {
    dlgTextEntryShowModal(global_wpt->Name, NAME_SIZE);
  }
  UpdateButtons();
}


static void OnCommentClicked(WindowControl *Sender) {
  if (buttonComment) {
    dlgTextEntryShowModal(global_wpt->Comment, COMMENT_SIZE);
  }
  UpdateButtons();
}

//
// 

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnCloseClicked),
  DeclearCallBackEntry(NULL)
};



void dlgWaypointEditShowModal(WAYPOINT *wpt) {
  if (!wpt) {
    return;
  }

  global_wpt = wpt;

#ifndef GNAV
  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgWaypointEdit_L.xml"));
    wf = dlgLoadFromXML(CallBackTable, 
                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_WAYPOINTEDIT_L"));
  } else
#endif
    {
    char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgWaypointEdit.xml"));
  wf = dlgLoadFromXML(CallBackTable, 
                      filename, 
		      hWndMainWindow,
		      TEXT("IDR_XML_WAYPOINTEDIT"));
    }

  if (wf) {

    buttonName = ((WndButton *)wf->FindByName(TEXT("cmdName")));
    if (buttonName) {
      buttonName->SetOnClickNotify(OnNameClicked);
    }

    buttonComment = ((WndButton *)wf->FindByName(TEXT("cmdComment")));
    if (buttonComment) {
      buttonComment->SetOnClickNotify(OnCommentClicked);
    }

    UpdateButtons();

    WndProperty* wp;
    bool sign;
    int dd,mm,ss;

    Units::LongitudeToDMS(wpt->Longitude,
                          &dd, &mm, &ss, &sign);
    if (!sign) { dd = -dd; }

    wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(dd);
      wp->RefreshDisplay();
    }
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

    Units::LatitudeToDMS(wpt->Latitude,
                         &dd, &mm, &ss, &sign);
    if (!sign) { dd = -dd; }

    wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(dd);
      wp->RefreshDisplay();
    }
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

    wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
    if (wp) {
      wp->GetDataField()->SetAsFloat(
                           iround(wpt->Altitude*ALTITUDEMODIFY));
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
      dfe->Set(0);
      if ((wpt->Flags & LANDPOINT)==LANDPOINT) {
        dfe->Set(2);
      } 
      if ((wpt->Flags & AIRPORT)==AIRPORT) {
        dfe->Set(1);
      }

      wp->RefreshDisplay();
    }

    ////

    wf->SetModalResult(mrCancle);

    if (wf->ShowModal()==mrOK) {

      ////
      
      double num=0;
      
      wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeD"));
      if (wp) {
        dd = wp->GetDataField()->GetAsInteger();
        if (dd<0) {
          sign = 0;
          dd = -dd;
        } else {
          sign = 1;
        }
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeM"));
      if (wp) {
        mm = wp->GetDataField()->GetAsInteger();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpLongitudeS"));
      if (wp) {
        ss = wp->GetDataField()->GetAsInteger();
      }
      num = dd+mm/60.0+ss/3600.0;
      if (!sign) {
        num = -num;
      }
      
      wpt->Longitude = num;
      
      wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeD"));
      if (wp) {
        dd = wp->GetDataField()->GetAsInteger();
        if (dd<0) {
          sign = 0;
          dd = -dd;
        } else {
          sign = 1;
        }
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeM"));
      if (wp) {
          mm = wp->GetDataField()->GetAsInteger();
      }
      wp = (WndProperty*)wf->FindByName(TEXT("prpLatitudeS"));
      if (wp) {
        ss = wp->GetDataField()->GetAsInteger();
      }
      num = dd+mm/60.0+ss/3600.0;
      if (!sign) {
        num = -num;
      }
      
      wpt->Latitude = num;
        
      wp = (WndProperty*)wf->FindByName(TEXT("prpAltitude"));
      if (wp) {
        ss = wp->GetDataField()->GetAsInteger();
        if (ss==0) {
          WaypointAltitudeFromTerrain(wpt);
        } else {
          wpt->Altitude = ss/ALTITUDEMODIFY;
        }
      }
      
      wp = (WndProperty*)wf->FindByName(TEXT("prpFlags"));
      if (wp) {
        int myflag = wp->GetDataField()->GetAsInteger();
        switch(myflag) {
        case 0:
          wpt->Flags = TURNPOINT;
          break;
        case 1:
          wpt->Flags = AIRPORT | TURNPOINT;
          break;
        case 2:
          wpt->Flags = LANDPOINT;
          break;
        default:
          wpt->Flags = 0;
        };
      }
    }

    delete wf;
  }
  wf = NULL;

}

