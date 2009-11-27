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
#include "Dialogs/Message.hpp"
#include "Blackboard.hpp"
#include "InfoBoxLayout.h"
#include "Airspace.h"
#include "AirspaceDatabase.hpp"
#include "AirspaceWarning.h"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "MainWindow.hpp"
#include "MapWindow.h"

#include <assert.h>

static int index_circle = -1;
static int index_area = -1;
static WndForm *wf=NULL;

static void OnAcknowledgeClicked(WindowControl * Sender){
  (void)Sender;

  TCHAR *Name = NULL;
  if (index_circle>=0) {
    Name = airspace_database.AirspaceCircle[index_circle].Name;
  } else if (index_area>=0) {
    Name = airspace_database.AirspaceArea[index_area].Name;
  }
  if (Name) {
    UINT answer;
    answer = MessageBoxX(Name,
			 gettext(TEXT("Acknowledge for day?")),
			 MB_YESNOCANCEL|MB_ICONQUESTION);
    if (answer == IDYES) {
      if (index_circle>=0) {
	AirspaceWarnListAdd(&XCSoarInterface::Basic(),
                            &XCSoarInterface::Calculated(),
                            &XCSoarInterface::SettingsComputer(),
                            XCSoarInterface::MapProjection(),
                            false, true, index_circle, true);
      } else if (index_area>=0) {
	AirspaceWarnListAdd(&XCSoarInterface::Basic(),
                            &XCSoarInterface::Calculated(),
                            &XCSoarInterface::SettingsComputer(),
                            XCSoarInterface::MapProjection(),
                            false, false, index_area, true);
      }
      wf->SetModalResult(mrOK);
    } else if (answer == IDNO) {
      // this will cancel a daily ack
      if (index_circle>=0) {
	AirspaceWarnListAdd(&XCSoarInterface::Basic(),
                            &XCSoarInterface::Calculated(),
                            &XCSoarInterface::SettingsComputer(),
                            XCSoarInterface::MapProjection(),
                            true, true, index_circle, true);
      } else if (index_area>=0) {
	AirspaceWarnListAdd(&XCSoarInterface::Basic(),
                            &XCSoarInterface::Calculated(),
                            &XCSoarInterface::SettingsComputer(),
                            XCSoarInterface::MapProjection(),
                            true, false, index_area, true);
      }
      wf->SetModalResult(mrOK);
    }
  }
}


static void OnCloseClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAcknowledgeClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

static double FLAltRounded(double alt) {
  int f = iround(alt/10)*10;
  return (double)f;
}

static void SetValues(void) {
  int atype = 0;
  AIRSPACE_ALT* top = NULL;
  AIRSPACE_ALT* base = NULL;
  TCHAR *name = 0;
  WndProperty* wp;
  TCHAR buffer[80];
  TCHAR buffer2[80];
  bool inside = false;
  double range = 0.0;
  double bearing;

  if (index_area >=0) {
    AIRSPACE_AREA &area = airspace_database.AirspaceArea[index_area];
    MapWindow &map_window = XCSoarInterface::main_window.map;

    atype = area.Type;
    top = &area.Top;
    base = &area.Base;
    name = area.Name;
    inside = airspace_database.InsideArea(XCSoarInterface::Basic().Location,
                                          index_area);
    range = airspace_database.RangeArea(XCSoarInterface::Basic().Location,
                                        index_area, &bearing,
                                        map_window);
  }

  if (index_circle >=0) {
    AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[index_circle];

    atype = circle.Type;
    top = &circle.Top;
    base = &circle.Base;
    name = circle.Name;
    inside = airspace_database.InsideCircle(XCSoarInterface::Basic().Location,
                                            index_circle);
    range = airspace_database.CircleDistance(XCSoarInterface::Basic().Location,
                                             index_circle);

    DistanceBearing(XCSoarInterface::Basic().Location, circle.Location,
		    NULL, &bearing);
    if (inside) {
      bearing = AngleLimit360(bearing+180);
    }
  }

  if (range<0) {
    range = -range;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpName"));
  if (wp) {
    wp->SetText(name);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpType"));
  if (wp) {
    switch (atype) {
    case RESTRICT:
      wp->SetText(gettext(TEXT("Restricted"))); break;
    case PROHIBITED:
      wp->SetText(gettext(TEXT("Prohibited"))); break;
    case DANGER:
      wp->SetText(gettext(TEXT("Danger Area"))); break;
    case CLASSA:
      wp->SetText(gettext(TEXT("Class A"))); break;
    case CLASSB:
      wp->SetText(gettext(TEXT("Class B"))); break;
    case CLASSC:
      wp->SetText(gettext(TEXT("Class C"))); break;
    case CLASSD:
      wp->SetText(gettext(TEXT("Class D"))); break;
    case CLASSE:
      wp->SetText(gettext(TEXT("Class E"))); break;
    case CLASSF:
      wp->SetText(gettext(TEXT("Class F"))); break;
    case NOGLIDER:
      wp->SetText(gettext(TEXT("No Glider"))); break;
    case CTR:
      wp->SetText(gettext(TEXT("CTR"))); break;
    case WAVE:
      wp->SetText(gettext(TEXT("Wave"))); break;
    default:
      wp->SetText(gettext(TEXT("Unknown")));
    }
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTop"));
  if (wp) {
    switch (top->Base){
    case abUndef:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("%.0f[m] %.0f[ft] [?]"),
		  (top->Altitude),
		  (top->Altitude*TOFEET));
      } else {
	_stprintf(buffer, TEXT("%.0f ft [?]"),
		  (top->Altitude*TOFEET));
      }
      break;
    case abMSL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("%.0f[m] %.0f[ft] MSL"),
		  top->Altitude, top->Altitude*TOFEET);
      } else {
	_stprintf(buffer, TEXT("%.0f ft MSL"),
		  top->Altitude*TOFEET);
      }
      break;
    case abAGL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("%.0f[m] %.0f[ft] AGL"),
		  top->AGL, top->AGL*TOFEET);
      } else {
	_stprintf(buffer, TEXT("%.0f ft AGL"),
		  top->AGL*TOFEET);
      }
      break;
    case abFL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("FL%.0f (%.0f[m] %.0f[ft])"),
		  top->FL, FLAltRounded(top->Altitude),
		  FLAltRounded(top->Altitude*TOFEET));
      } else {
	_stprintf(buffer, TEXT("FL%.0f (%.0f ft)"),
		  top->FL, FLAltRounded(top->Altitude*TOFEET));
      }
      break;
    }
    wp->SetText(buffer);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBase"));
  if (wp) {
    switch (base->Base){
    case abUndef:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("%.0f[m] %.0f[ft] [?]"),
		  base->Altitude, base->Altitude*TOFEET);
      } else {
	_stprintf(buffer, TEXT("%.0f ft [?]"),
		  base->Altitude*TOFEET);
      }
      break;
    case abMSL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("%.0f[m] %.0f[ft] MSL"),
		  base->Altitude, base->Altitude*TOFEET);
      } else {
	_stprintf(buffer, TEXT("%.0f ft MSL"),
		  base->Altitude*TOFEET);
      }
      break;
    case abAGL:
      if (base->Altitude == 0) {
        _stprintf(buffer, TEXT("SFC"));
      } else {
	if (Units::GetUserAltitudeUnit() == unMeter) {
	  _stprintf(buffer, TEXT("%.0f[m] %.0f[ft] AGL"),
		    base->AGL, base->AGL*TOFEET);
	} else {
	  _stprintf(buffer, TEXT("%.0f ft AGL"),
		    base->AGL*TOFEET);
	}
      }
      break;
    case abFL:
      if (Units::GetUserAltitudeUnit() == unMeter) {
	_stprintf(buffer, TEXT("FL %.0f (%.0f[m] %.0f[ft])"),
		  base->FL, FLAltRounded(base->Altitude),
		  FLAltRounded(base->Altitude*TOFEET));
      } else {
	_stprintf(buffer, TEXT("FL%.0f (%.0f ft)"),
		  base->FL, FLAltRounded(base->Altitude*TOFEET));
      }
      break;
    }
    wp->SetText(buffer);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    if (inside) {
      wp->SetCaption(gettext(TEXT("Inside")));
    }
    Units::FormatUserDistance(range, buffer, 20);
    _stprintf(buffer2, TEXT(" %d")TEXT(DEG), iround(bearing));
    _tcscat(buffer, buffer2);
    wp->SetText(buffer);
    wp->RefreshDisplay();
  }
}


void dlgAirspaceDetails(int the_circle, int the_area) {
  index_circle = the_circle;
  index_area = the_area;
  if ((index_area<=0) && (index_circle <=0)) {
    return;
  }

  wf = dlgLoadFromXML(CallBackTable,
                      TEXT("dlgAirspaceDetails.xml"),
		      XCSoarInterface::main_window,
		      TEXT("IDR_XML_AIRSPACEDETAILS"));

  if (!wf) return;

  assert(wf!=NULL);

  SetValues();

  wf->ShowModal();

  delete wf;
  wf = NULL;
  return;
}


/*


			distance,
                    Units::GetDistanceName()

  wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(distance);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

*/
