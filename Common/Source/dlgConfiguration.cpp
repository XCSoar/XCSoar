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
#include <Aygshell.h>

#include "XCSoar.h"
#include "MapWindow.h"
#include "Terrain.h"
#include "GaugeFLARM.h"

#include "WindowControls.h"
#include "Statistics.h"
#include "Externs.h"
#include "McReady.h"
#include "dlgTools.h"
#include "device.h"
#include "Process.h"
#include "McReady.h"
#include "Utils.h"
#include "InfoBoxLayout.h"


static bool changed = false;
static bool taskchanged = false;
static bool requirerestart = false;
static bool utcchanged = false;
static int page=0;
static WndForm *wf=NULL;
static WndFrame *wConfig1=NULL;
static WndFrame *wConfig2=NULL;
static WndFrame *wConfig3=NULL;
static WndFrame *wConfig4=NULL;
static WndFrame *wConfig5=NULL;
static WndFrame *wConfig6=NULL;
static WndFrame *wConfig7=NULL;
static WndFrame *wConfig8=NULL;
static WndFrame *wConfig9=NULL;
static WndFrame *wConfig10=NULL;
static WndFrame *wConfig11=NULL;
static WndFrame *wConfig12=NULL;
static WndFrame *wConfig13=NULL;
static WndFrame *wConfig14=NULL;
static WndFrame *wConfig15=NULL;
static WndFrame *wConfig16=NULL;
static WndFrame *wConfig17=NULL;
static WndButton *buttonPilotName=NULL;
static WndButton *buttonAircraftType=NULL;
static WndButton *buttonAircraftRego=NULL;



static void UpdateButtons(void) {
  TCHAR text[120];
  TCHAR val[100];
  if (buttonPilotName) {
    GetRegistryString(szRegistryPilotName, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(text,TEXT("Pilot name: (blank)"));
    } else {
      _stprintf(text,TEXT("Pilot name: %s"),val);
    }
    buttonPilotName->SetCaption(text);
  }
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(text,TEXT("Aircraft type: (blank)"));
    } else {
      _stprintf(text,TEXT("Aircraft type: %s"),val);
    }
    buttonAircraftType->SetCaption(text);
  }
  if (buttonAircraftRego) {
    GetRegistryString(szRegistryAircraftRego, val, 100);
    if (_tcslen(val)<=0) {
      _stprintf(text,TEXT("Aircraft rego: (blank)"));
    } else {
      _stprintf(text,TEXT("Aircraft rego: %s"),val);
    }
    buttonAircraftRego->SetCaption(text);
  }
}

extern bool EnableAnimation;

#define NUMPAGES 17

static void NextPage(int Step){
  page += Step;
  if (page>=NUMPAGES) { page=0; }
  if (page<0) { page=NUMPAGES-1; }
  switch(page) {
  case 0:
    wf->SetCaption(TEXT("1 Airspace"));
    break;
  case 1:
    wf->SetCaption(TEXT("2 Map Display"));
    break;
  case 2:
    wf->SetCaption(TEXT("3 Glide Computer"));
    break;
  case 3:
    wf->SetCaption(TEXT("4 Safety factors"));
    break;
  case 4:
    wf->SetCaption(TEXT("5 Polar"));
    break;
  case 5:
    wf->SetCaption(TEXT("6 Devices"));
    break;
  case 6:
    wf->SetCaption(TEXT("7 Units"));
    break;
  case 7:
    wf->SetCaption(TEXT("8 Interface"));
    break;
  case 8:
    wf->SetCaption(TEXT("9 Appearance"));
    break;
  case 9:
    wf->SetCaption(TEXT("10 Vario Gauge"));
    break;
  case 10:
    wf->SetCaption(TEXT("11 Task"));
    break;
  case 11:
    wf->SetCaption(TEXT("12 Task rules"));
    break;
  case 12:
    wf->SetCaption(TEXT("13 InfoBox Circling"));
    break;
  case 13:
    wf->SetCaption(TEXT("14 InfoBox Cruise"));
    break;
  case 14:
    wf->SetCaption(TEXT("15 InfoBox Final Glide"));
    break;
  case 15:
    wf->SetCaption(TEXT("16 InfoBox Auxiliary"));
    break;
  case 16:
    wf->SetCaption(TEXT("17 Logger"));
    break;
  }
  wConfig1->SetVisible(page == 0);
  wConfig2->SetVisible(page == 1);
  wConfig3->SetVisible(page == 2);
  wConfig4->SetVisible(page == 3);
  wConfig5->SetVisible(page == 4);
  wConfig6->SetVisible(page == 5);
  wConfig7->SetVisible(page == 6);
  wConfig8->SetVisible(page == 7);
  wConfig9->SetVisible(page == 8);
  wConfig10->SetVisible(page == 9);
  wConfig11->SetVisible(page == 10);
  wConfig12->SetVisible(page == 11);
  wConfig13->SetVisible(page == 12);
  wConfig14->SetVisible(page == 13);
  wConfig15->SetVisible(page == 14);
  wConfig16->SetVisible(page == 15);
  wConfig17->SetVisible(page == 16);
}



static void OnVarioClicked(WindowControl * Sender){
  changed = dlgConfigurationVarioShowModal();

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  wf->FocusNext(NULL);

}


static void OnAircraftRegoClicked(WindowControl *Sender) {
  TCHAR Temp[100];
  if (buttonAircraftRego) {
    GetRegistryString(szRegistryAircraftRego,Temp,100);
    dlgTextEntryShowModal(Temp);
    SetRegistryString(szRegistryAircraftRego,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnAircraftTypeClicked(WindowControl *Sender) {
  TCHAR Temp[100];
  if (buttonAircraftType) {
    GetRegistryString(szRegistryAircraftType,Temp,100);
    dlgTextEntryShowModal(Temp);
    SetRegistryString(szRegistryAircraftType,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnPilotNameClicked(WindowControl *Sender) {
  TCHAR Temp[100];
  if (buttonPilotName) {
    GetRegistryString(szRegistryPilotName,Temp,100);
    dlgTextEntryShowModal(Temp);
    SetRegistryString(szRegistryPilotName,Temp);
    changed = true;
  }
  UpdateButtons();
}


static void OnAirspaceColoursClicked(WindowControl * Sender){
  dlgAirspaceShowModal(true);
}

static void OnAirspaceModeClicked(WindowControl * Sender){
  dlgAirspaceShowModal(false);
}

static void OnNextClicked(WindowControl * Sender){
  NextPage(+1);
}

static void OnPrevClicked(WindowControl * Sender){
  NextPage(-1);
}

static void OnCloseClicked(WindowControl * Sender){
  wf->SetModalResult(mrOK);
}

static int FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam){
  switch(wParam & 0xffff){
    case '6':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdPrev")))->GetHandle());
      NextPage(-1);
      //((WndButton *)wf->FindByName(TEXT("cmdPrev")))->SetFocused(true, NULL);
    return(0);
    case '7':
      SetFocus(((WndButton *)wf->FindByName(TEXT("cmdNext")))->GetHandle());
      NextPage(+1);
      //((WndButton *)wf->FindByName(TEXT("cmdNext")))->SetFocused(true, NULL);
    return(0);
  }
  return(1);
}

static void SetLocalTime(void) {
  WndProperty* wp;
  TCHAR temp[20];
  Units::TimeToText(temp,
		    (int)TimeLocal((int)(GPS_INFO.Time)));

  wp = (WndProperty*)wf->FindByName(TEXT("prpLocalTime"));
  if (wp) {
    wp->SetText(temp);
    wp->RefreshDisplay();
  }
}

static void OnUTCData(DataField *Sender, DataField::DataAccessKind_t Mode){
//  WndProperty* wp;
  int ival;

  switch(Mode){
    case DataField::daGet:
    break;
    case DataField::daPut:
    case DataField::daChange:
      ival = iround(Sender->GetAsFloat()*3600.0);
      if (UTCOffset != ival) {
	UTCOffset = ival;
	utcchanged = true;
      }
      SetLocalTime();
    break;
  }

}


static void OnInfoBoxHelp(WindowControl * Sender){
  WndProperty *wp = (WndProperty*)Sender;
  int type = wp->GetDataField()->GetAsInteger();
  TCHAR caption[100];
  TCHAR mode[100];
  switch (page) {
  case 12:
    _tcscpy(mode,TEXT("circling"));
    break;
  case 13:
    _tcscpy(mode,TEXT("cruise"));
    break;
  case 14:
    _tcscpy(mode,TEXT("final glide"));
    break;
  case 15:
    _tcscpy(mode,TEXT("auxiliary"));
    break;
  default:
    _tcscpy(mode,TEXT("Error"));
    return;
  }
  _stprintf(caption, TEXT("InfoBox %s in %s mode"),
	    wp->GetCaption(), mode);
  switch(type) {
  case 0:
    dlgHelpShowModal(caption, TEXT("[Height GPS]\r\nThis is the height above mean sea level reported by the GPS.\r\n Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."));
    break;
  case 1:
    dlgHelpShowModal(caption, TEXT("[Height AGL]\r\nThis is the navigation altitude minus the terrain height obtained from the terrain file.  The value is coloured red when the glider is below the terrain safety clearance height."));
    break;
  case 2:
    dlgHelpShowModal(caption, TEXT("[Thermal last 30 sec]\r\nA 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."));
    break;
  case 3:
    dlgHelpShowModal(caption, TEXT("[Bearing]\r\nTrue bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."));
    break;
  case 4:
    dlgHelpShowModal(caption, TEXT("[L/D instantaneous]\r\nInstantaneous glide ratio, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds.  Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."));
    break;
  case 5:
    dlgHelpShowModal(caption, TEXT("[L/D cruise]\r\nThe distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal.  Negative values indicate climbing cruise (height gain since leaving the last thermal).  If the vertical speed is close to zero, the displayed value is '---'."));
    break;
  case 6:
    dlgHelpShowModal(caption, TEXT("[Speed ground]\r\nGround speed measured by the GPS.  If this infobox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."));
    break;
  case 7:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Average]\r\nTotal altitude gain/loss in the last thermal divided by the time spent circling."));
    break;
  case 8:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Gain]\r\nTotal altitude gain/loss in the last thermal."));
    break;
  case 9:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Time]\r\nTime spent circling in the last thermal."));
    break;
  case 10:
    dlgHelpShowModal(caption, TEXT("[MacCready Setting]\r\nThe current MacCready setting.  This infobox also shows whether MacCready is manual or auto.  (Touchscreen/PC only) Also used to adjust the MacCready Setting if the infobox is active, by using the up/down cursor keys."));
    break;
  case 11:
    dlgHelpShowModal(caption, TEXT("[Next Distance]\r\nThe distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."));
    break;
  case 12:
    dlgHelpShowModal(caption, TEXT("[Next Altitude Difference]\r\nArrival altitude at the next waypoint relative to the safety arrival altitude."));
    break;
  case 13:
    dlgHelpShowModal(caption, TEXT("[Next Altitude Required]\r\nAltitude required to reach the next turn point."));
    break;
  case 14:
    dlgHelpShowModal(caption, TEXT("[Next Waypoint]\r\n The name of the currently selected turn point.  When this infobox is active, using the up/down cursor keys selects the next/previous waypoint in the task.  (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details."));
    break;
  case 15:
    dlgHelpShowModal(caption, TEXT("[Final Altitude Difference]\r\nArrival altitude at the final task turn point relative to the safety arrival altitude."));
    break;
  case 16:
    dlgHelpShowModal(caption, TEXT("[Final Altitude Required]\r\nAltitude required to finish the task."));
    break;
  case 17:
    dlgHelpShowModal(caption, TEXT("[Speed Task Average]\r\nAverage cross country speed while on current task, compensated for altitude."));
    break;
  case 18:
    dlgHelpShowModal(caption, TEXT("[Final Distance]\r\nDistance to finish around remaining turn points."));
    break;
  case 19:
    dlgHelpShowModal(caption, TEXT("[Final L/D]\r\nThe required glide ratio to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival altitude.  Negative values indicate a climb is necessary to finish. If the height required is close to zero, the displayed value is '---'."));
    break;
  case 20:
    dlgHelpShowModal(caption, TEXT("[Terrain Elevation]\r\nThis is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."));
    break;
  case 21:
    dlgHelpShowModal(caption, TEXT("[Thermal Average]\r\nAltitude gained/lost in the current thermal, divided by time spent thermaling."));
    break;
  case 22:
    dlgHelpShowModal(caption, TEXT("[Thermal Gain]\r\nThe altitude gained/lost in the current thermal."));
    break;
  case 23:
    dlgHelpShowModal(caption, TEXT("[Track]\r\nMagnetic track reported by the GPS.  (Touchscreen/PC only) If this infobox is active in simulation mode, pressing the up and down  arrows adjusts the track."));
    break;
  case 24:
    dlgHelpShowModal(caption, TEXT("[Vario]\r\nInstantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."));
    break;
  case 25:
    dlgHelpShowModal(caption, TEXT("[Wind Speed]\r\nWind speed estimated by XCSoar.  (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the infobox is active.  Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts."));
    break;
  case 26:
    dlgHelpShowModal(caption, TEXT("[Wind Bearing]\r\nWind bearing estimated by XCSoar.  (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the infobox is active."));
    break;
  case 27:
    dlgHelpShowModal(caption, TEXT("[AA Time]\r\nAssigned Area Task time remaining."));
    break;
  case 28:
    dlgHelpShowModal(caption, TEXT("[AA Distance Max]\r\nAssigned Area Task maximum distance possible for remainder of task."));
    break;
  case 29:
    dlgHelpShowModal(caption, TEXT("[AA Distance Min]\r\nAssigned Area Task minimum distance possible for remainder of task."));
    break;
  case 30:
    dlgHelpShowModal(caption, TEXT("[AA Speed Max]\r\nAssigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."));
    break;
  case 31:
    dlgHelpShowModal(caption, TEXT("[AA Speed Min]\r\nAssigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."));
    break;
  case 32:
    dlgHelpShowModal(caption, TEXT("[Airspeed IAS]\r\nIndicated Airspeed reported by a supported external intelligent vario."));
    break;
  case 33:
    dlgHelpShowModal(caption, TEXT("[Pressure Altitude]\r\nThis is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario."));
    break;
  case 34:
    dlgHelpShowModal(caption, TEXT("[Speed MacReady]\r\nThe MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude.  In final glide mode, this speed-to-fly is calculated for descent."));
    break;
  case 35:
    dlgHelpShowModal(caption, TEXT("[Percentage climb]\r\nPercentage of time spent in climb mode."));
    break;
  case 36:
    dlgHelpShowModal(caption, TEXT("[Time of flight]\r\nTime elapsed since takeoff was detected."));
    break;
  case 37:
    dlgHelpShowModal(caption, TEXT("[G load]\r\nMagnitude of G loading reported by a supported external intelligent vario.  This value is negative for pitch-down manoeuvres."));
    break;
  case 38:
    dlgHelpShowModal(caption, TEXT("[Next L/D]\r\nThe required glide ratio to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival altitude.  Negative values indicate a climb is necessary to reach the waypoint.  If the height required is close to zero, the displayed value is '---'."));
    break;
  case 39:
    dlgHelpShowModal(caption, TEXT("[Time local]\r\nGPS time expressed in local time zone."));
    break;
  case 40:
    dlgHelpShowModal(caption, TEXT("[Time UTC]\r\nGPS time expressed in UTC."));
    break;
  case 41:
    dlgHelpShowModal(caption, TEXT("[Task Time To Go]\r\nEstimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 42:
    dlgHelpShowModal(caption, TEXT("[Next Time To Go]\r\nEstimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 43:
    dlgHelpShowModal(caption, TEXT("[Speed Dolphin]\r\nThe instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing.  In cruise flight mode, this speed-to-fly is calculated for maintaining altitude.  In final glide mode, this speed-to-fly is calculated for descent.  In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected).  When Block mode speed to fly is selected, this infobox displays the MacCready speed."));
    break;
  case 44:
    dlgHelpShowModal(caption, TEXT("[Netto Vario]\r\nInstantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate.  Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."));
    break;
  case 45:
    dlgHelpShowModal(caption, TEXT("[Task Arrival Time]\r\nEstimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 46:
    dlgHelpShowModal(caption, TEXT("[Next Arrival Time]\r\nEstimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 47:
    dlgHelpShowModal(caption, TEXT("[Bearing Difference]\r\nThe difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector.  GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present.  Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint.  This bearing takes into account the curvature of the Earth."));
    break;
  case 48:
    dlgHelpShowModal(caption, TEXT("[Outside Air Temperature]\r\nOutside air temperature measured by a probe if supported by a connected intelligent variometer."));
    break;
  case 49:
    dlgHelpShowModal(caption, TEXT("[Relative Humidity]\r\nRelative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."));
    break;
  case 50:
    dlgHelpShowModal(caption, TEXT("[Forecast Temperature]\r\nForecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe.  (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."));
    break;
  case 51:
    dlgHelpShowModal(caption, TEXT("[AA Distance Tgt]\r\nAssigned Area Task distance around target points for remainder of task."));
    break;
  case 52:
    dlgHelpShowModal(caption, TEXT("[AA Speed Tgt]\r\nAssigned Area Task average speed achievable around target points remaining in minimum AAT time."));
    break;
  case 53:
    dlgHelpShowModal(caption, TEXT("[L/D vario]\r\nInstantaneous glide ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer.  Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."));
    break;
  case 54:
    dlgHelpShowModal(caption, TEXT("[Airspeed TAS]\r\nTrue Airspeed reported by a supported external intelligent vario."));
    break;
  case 55:
    dlgHelpShowModal(caption, TEXT("[Own Team Code]\r\nThe current Team code for this aircraft.  Use this to report to other team members."));
    break;
  case 56:
    dlgHelpShowModal(caption, TEXT("[Team Bearing]\r\nThe bearing to the team aircraft location at the last team code report."));
    break;
  case 57:
    dlgHelpShowModal(caption, TEXT("[Team Bearing Diff]\r\nThe relative bearing to the team aircraft location at the last reported team code."));
    break;
  case 58:
    dlgHelpShowModal(caption, TEXT("[Team range]\r\nThe range to the team aircraft location at the last reported team code."));
    break;
  case 59:
    dlgHelpShowModal(caption, TEXT("[Speed Task Instantaneous]\r\nInstantaneous cross country speed while on current task, compensated for altitude."));
    break;
  };
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnAirspaceColoursClicked),
  DeclearCallBackEntry(OnAirspaceModeClicked),
  DeclearCallBackEntry(OnUTCData),
  DeclearCallBackEntry(OnNextClicked),
  DeclearCallBackEntry(OnPrevClicked),
  DeclearCallBackEntry(OnVarioClicked),
  DeclearCallBackEntry(OnInfoBoxHelp),
  DeclearCallBackEntry(NULL)
};


extern SCREEN_INFO Data_Options[];
extern int NUMSELECTSTRINGS;
extern int InfoType[];

static void SetInfoBoxSelector(TCHAR *name, int item, int mode)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (int i=0; i<NUMSELECTSTRINGS; i++) {
      dfe->addEnumText(Data_Options[i].Description);
    }
    int it=0;

    switch(mode) {
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 1: // climb
      it = (InfoType[item])& 0xff;
      break;
    case 2: // final glide
      it = (InfoType[item]>>16)& 0xff;
      break;
    case 3: // aux
      it = (InfoType[item]>>24)& 0xff;
      break;
    };
    dfe->Set(it);
    wp->RefreshDisplay();
  }
}


void GetInfoBoxSelector(TCHAR *name, int item, int mode)
{
  WndProperty *wp;
  wp = (WndProperty*)wf->FindByName(name);
  if (wp) {
    int itnew = wp->GetDataField()->GetAsInteger();
    int it=0;

    switch(mode) {
    case 0: // cruise
      it = (InfoType[item]>>8)& 0xff;
      break;
    case 1: // climb
      it = (InfoType[item])& 0xff;
      break;
    case 2: // final glide
      it = (InfoType[item]>>16)& 0xff;
      break;
    case 3: // aux
      it = (InfoType[item]>>24)& 0xff;
      break;
    };

    if (it != itnew) {

      changed = true;

      switch(mode) {
      case 0: // cruise
	InfoType[item] &= 0xffff00ff;
	InfoType[item] += (itnew<<8);
	break;
      case 1: // climb
	InfoType[item] &= 0xffffff00;
	InfoType[item] += itnew;
	break;
      case 2: // final glide
	InfoType[item] &= 0xff00ffff;
	InfoType[item] += (itnew<<16);
	break;
      case 3: // aux
	InfoType[item] &= 0x00ffffff;
	InfoType[item] += (itnew<<24);
	break;
      };
      StoreType(item,InfoType[item]);
    }
  }
}




extern TCHAR *PolarLabels[];

void dlgConfigurationShowModal(void){

  WndProperty *wp;

#ifndef GNAV
  if (!InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgConfiguration_L.xml"));
    wf = dlgLoadFromXML(CallBackTable,

                        filename,
                        hWndMainWindow,
                        TEXT("IDR_XML_CONFIGURATION_L"));
  } else
#endif
    {
    char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgConfiguration.xml"));
  wf = dlgLoadFromXML(CallBackTable,

                      filename,
		      hWndMainWindow,
		      TEXT("IDR_XML_CONFIGURATION"));
    }

  if (!wf) return;

  wf->SetKeyDownNotify(FormKeyDown);

  ((WndButton *)wf->FindByName(TEXT("cmdClose")))->SetOnClickNotify(OnCloseClicked);

  wConfig1    = ((WndFrame *)wf->FindByName(TEXT("frmAirspace")));
  wConfig2    = ((WndFrame *)wf->FindByName(TEXT("frmDisplay")));
  wConfig3    = ((WndFrame *)wf->FindByName(TEXT("frmFinalGlide")));
  wConfig4    = ((WndFrame *)wf->FindByName(TEXT("frmSafety")));
  wConfig5    = ((WndFrame *)wf->FindByName(TEXT("frmPolar")));
  wConfig6    = ((WndFrame *)wf->FindByName(TEXT("frmComm")));
  wConfig7    = ((WndFrame *)wf->FindByName(TEXT("frmUnits")));
  wConfig8    = ((WndFrame *)wf->FindByName(TEXT("frmInterface")));
  wConfig9    = ((WndFrame *)wf->FindByName(TEXT("frmAppearance")));
  wConfig10    = ((WndFrame *)wf->FindByName(TEXT("frmVarioAppearance")));
  wConfig11    = ((WndFrame *)wf->FindByName(TEXT("frmTask")));
  wConfig12    = ((WndFrame *)wf->FindByName(TEXT("frmTaskRules")));
  wConfig13    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCircling")));
  wConfig14    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxCruise")));
  wConfig15    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxFinalGlide")));
  wConfig16    = ((WndFrame *)wf->FindByName(TEXT("frmInfoBoxAuxiliary")));
  wConfig17    = ((WndFrame *)wf->FindByName(TEXT("frmLogger")));

  ASSERT(wConfig1!=NULL);
  ASSERT(wConfig2!=NULL);
  ASSERT(wConfig3!=NULL);
  ASSERT(wConfig4!=NULL);
  ASSERT(wConfig5!=NULL);
  ASSERT(wConfig6!=NULL);
  ASSERT(wConfig7!=NULL);
  ASSERT(wConfig8!=NULL);
  ASSERT(wConfig9!=NULL);
  ASSERT(wConfig10!=NULL);
  ASSERT(wConfig11!=NULL);
  ASSERT(wConfig12!=NULL);
  ASSERT(wConfig13!=NULL);
  ASSERT(wConfig14!=NULL);
  ASSERT(wConfig15!=NULL);
  ASSERT(wConfig16!=NULL);
  ASSERT(wConfig17!=NULL);

  buttonPilotName = ((WndButton *)wf->FindByName(TEXT("cmdPilotName")));
  if (buttonPilotName) {
    buttonPilotName->SetOnClickNotify(OnPilotNameClicked);
  }
  buttonAircraftType = ((WndButton *)wf->FindByName(TEXT("cmdAircraftType")));
  if (buttonAircraftType) {
    buttonAircraftType->SetOnClickNotify(OnAircraftTypeClicked);
  }
  buttonAircraftRego = ((WndButton *)wf->FindByName(TEXT("cmdAircraftRego")));
  if (buttonAircraftRego) {
    buttonAircraftRego->SetOnClickNotify(OnAircraftRegoClicked);
  }

  UpdateButtons();

  //////////
  /*
  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  wf = NULL;

  return;
  */
  ///////////////////

  // TODO: all appearance variables

  TCHAR *COMMPort[] = {TEXT("COM1"),TEXT("COM2"),TEXT("COM3"),TEXT("COM4"),TEXT("COM5"),TEXT("COM6"),TEXT("COM7"),TEXT("COM8"),TEXT("COM9"),TEXT("COM10")};
  TCHAR *tSpeed[] = {TEXT("1200"),TEXT("2400"),TEXT("4800"),TEXT("9600"),TEXT("19200"),TEXT("38400"),TEXT("57600"),TEXT("115200")};
  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};

  DWORD dwPortIndex1 = 0;
  DWORD dwSpeedIndex1 = 2;
  DWORD dwPortIndex2 = 0;
  DWORD dwSpeedIndex2 = 2;

  int i;
  int dwDeviceIndex1=0;
  int dwDeviceIndex2=0;
  TCHAR DeviceName[DEVNAMESIZE+1];

  ReadPort1Settings(&dwPortIndex1,&dwSpeedIndex1);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<10; i++) {
      dfe->addEnumText(COMMPort[i]);
    }
    dfe->Set(dwPortIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText(tSpeed[i]);
    }
    dfe->Set(dwSpeedIndex1);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice1"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Generic"));
    for (i=0; i<DeviceRegisterCount; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText(DeviceName);
      if (devA() != NULL){
	if (_tcscmp(DeviceName, devA()->Name) == 0)
	  dwDeviceIndex1 = i+1;
      }
    }
    dfe->Set(dwDeviceIndex1);
    wp->RefreshDisplay();
  }

  ReadPort2Settings(&dwPortIndex2,&dwSpeedIndex2);

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<10; i++) {
      dfe->addEnumText(COMMPort[i]);
    }
    dfe->Set(dwPortIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<8; i++) {
      dfe->addEnumText(tSpeed[i]);
    }
    dfe->Set(dwSpeedIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice2"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Generic"));
    for (i=0; i<DeviceRegisterCount; i++) {
      devRegisterGetName(i, DeviceName);
      dfe->addEnumText(DeviceName);
      if (devB() != NULL){
	if (_tcscmp(DeviceName, devB()->Name) == 0)
	  dwDeviceIndex2 = i+1;
      }
    }
    dfe->Set(dwDeviceIndex2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("All on"));
    dfe->addEnumText(TEXT("Clip"));
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("All below"));
    dfe->Set(AltitudeMode);
    wp->RefreshDisplay();
  }
  //

  wp = (WndProperty*)wf->FindByName(TEXT("prpUTCOffset"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(UTCOffset/3600.0));
    wp->RefreshDisplay();
  }
  SetLocalTime();

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ClipAltitude*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(AltWarningMargin*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoZoom"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::AutoZoom);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::bAirspaceBlackOutline);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLockSettingsInFlight"));
  if (wp) {
    wp->GetDataField()->Set(LockSettingsInFlight);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDebounceTimeout"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(debounceTimeout);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMDisplay"));
  if (wp) {
    wp->GetDataField()->Set(EnableFLARMDisplay);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceWarnings"));
  if (wp) {
    bool aw = AIRSPACEWARNINGS != 0;
    wp->GetDataField()->Set(aw);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWarningTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(WarningTime);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledgementTime"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(AcknowledgementTime);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointLabels"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Names"));
    dfe->addEnumText(TEXT("Numbers"));
    dfe->addEnumText(TEXT("First 5"));
    dfe->addEnumText(TEXT("None"));
    dfe->addEnumText(TEXT("First 3"));
    dfe->addEnumText(TEXT("Names in task"));
    dfe->Set(DisplayTextType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTerrain"));
  if (wp) {
    wp->GetDataField()->Set(EnableTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTopology"));
  if (wp) {
    wp->GetDataField()->Set(EnableTopology);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpCirclingZoom"));
  if (wp) {
    wp->GetDataField()->Set(CircleZoom);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOrientation"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Track up"));
    dfe->addEnumText(TEXT("North up"));
    dfe->addEnumText(TEXT("North circling"));
    dfe->addEnumText(TEXT("Target circling"));
    dfe->Set(DisplayOrientation);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMenuTimeout"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MenuTimeoutMax/2);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDEARRIVAL));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDEBREAKOFF));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(ALTITUDEMODIFY*SAFETYALTITUDETERRAIN));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    wp->GetDataField()->Set(FinalGlideTerrain);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableNavBaroAltitude"));
  if (wp) {
    wp->GetDataField()->Set(EnableNavBaroAltitude);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Manual"));
    dfe->addEnumText(TEXT("Circling"));
    dfe->addEnumText(TEXT("ZigZag"));
    dfe->addEnumText(TEXT("Both"));
    wp->GetDataField()->Set(AutoWindMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoMcMode"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Final glide"));
    dfe->addEnumText(TEXT("Average climb"));
    dfe->addEnumText(TEXT("Both"));
    wp->GetDataField()->Set(AutoMcMode);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointsOutOfRange"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Ask"));
    dfe->addEnumText(TEXT("Include"));
    dfe->addEnumText(TEXT("Exclude"));
    wp->GetDataField()->Set(WaypointsOutOfRange);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoForceFinalGlide"));
  if (wp) {
    wp->GetDataField()->Set(AutoForceFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBlockSTF"));
  if (wp) {
    wp->GetDataField()->Set(EnableBlockSTF);
    wp->RefreshDisplay();
  }

  ////

  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCRules"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Sprint"));
    dfe->addEnumText(TEXT("Triangle"));
    dfe->addEnumText(TEXT("Classic"));
    dfe->Set(OLCRules);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpHandicap"));
  if (wp) {
    wp->GetDataField()->SetAsInteger(Handicap);
    wp->RefreshDisplay();
  }

  ////

  DWORD Speed = 1; // default is knots
  DWORD TaskSpeed = 2; // default is kph
  DWORD Distance = 2; // default is km
  DWORD Lift = 0;
  DWORD Altitude = 0; //default ft

  if(GetFromRegistry(szRegistrySpeedUnitsValue,&Speed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistrySpeedUnitsValue, Speed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Statue"));
    dfe->addEnumText(TEXT("Nautical"));
    dfe->addEnumText(TEXT("Metric"));
    dfe->Set(Speed);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryTaskSpeedUnitsValue,&TaskSpeed)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsTaskSpeed"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Statue"));
    dfe->addEnumText(TEXT("Nautical"));
    dfe->addEnumText(TEXT("Metric"));
    dfe->Set(TaskSpeed);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryDistanceUnitsValue,&Distance)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryDistanceUnitsValue, Distance);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Statute"));
    dfe->addEnumText(TEXT("Nautical"));
    dfe->addEnumText(TEXT("Metric"));
    dfe->Set(Distance);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Feet"));
    dfe->addEnumText(TEXT("Meters"));
    dfe->Set(Altitude);
    wp->RefreshDisplay();
  }

  if(GetFromRegistry(szRegistryLiftUnitsValue,&Lift)!=ERROR_SUCCESS) {
    SetToRegistry(szRegistryLiftUnitsValue, Lift);
    changed = true;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Knots"));
    dfe->addEnumText(TEXT("M/s"));
    dfe->Set(Lift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
  if (wp) {
    wp->GetDataField()->Set(MapWindow::EnableTrailDrift);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpThermalLocator"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("OFF"));
    dfe->addEnumText(TEXT("Circle at center"));
    dfe->addEnumText(TEXT("Pan to center"));
    dfe->Set(EnableThermalLocator);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSetSystemTimeFromGPS"));
  if (wp) {
    wp->GetDataField()->Set(SetSystemTimeFromGPS);
    wp->RefreshDisplay();
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpAbortSafetyUseCurrent"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::AbortSafetyUseCurrent);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyMacCready"));
  if (wp) {
    wp->GetDataField()->Set(GlidePolar::SafetyMacCready*LIFTMODIFY);
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAnimation"));
  if (wp) {
    wp->GetDataField()->Set(EnableAnimation);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Off"));
    dfe->addEnumText(TEXT("Long"));
    dfe->addEnumText(TEXT("Short"));
    dfe->Set(TrailActive);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(SPEEDMODIFY*SAFTEYSPEED));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarType"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    for (i=0; i<NUMPOLARS; i++) {
      dfe->addEnumText(PolarLabels[i]);
    }
    i=0;
    bool ok = true;
    while (ok) {
      TCHAR *name;
      name = GetWinPilotPolarInternalName(i);
      if (!name) {
	ok=false;
      } else {
	dfe->addEnumText(name);
      }
      i++;
    }
    dfe->Set(POLARID);
    wp->RefreshDisplay();
  }

  TCHAR temptext[MAX_PATH];

  TCHAR szPolarFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryPolarFile, szPolarFile, MAX_PATH);
  _tcscpy(temptext,szPolarFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.plr"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szAirspaceFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szAdditionalAirspaceFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAdditionalAirspaceFile,
		    szAdditionalAirspaceFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalAirspaceFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szWaypointFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
  _tcscpy(temptext,szWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szAdditionalWaypointFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAdditionalWayPointFile,
		    szAdditionalWaypointFile, MAX_PATH);
  _tcscpy(temptext,szAdditionalWaypointFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szTerrainFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
  _tcscpy(temptext,szTerrainFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.dat"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szTopologyFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
  _tcscpy(temptext,szTopologyFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.tpl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szAirfieldFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
  _tcscpy(temptext,szAirfieldFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.txt"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szLanguageFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryLanguageFile, szLanguageFile, MAX_PATH);
  _tcscpy(temptext,szLanguageFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcl"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szStatusFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryStatusFile, szStatusFile, MAX_PATH);
  _tcscpy(temptext,szStatusFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xcs"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  TCHAR szInputFile[MAX_PATH] = TEXT("\0");
  GetRegistryString(szRegistryInputFile, szInputFile, MAX_PATH);
  _tcscpy(temptext,szInputFile);
  ExpandLocalPath(temptext);
  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    dfe->ScanDirectoryTop(TEXT("*.xci"));
    dfe->Lookup(temptext);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppStatusMessageAlignment"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Center"));
    dfe->addEnumText(TEXT("Topleft"));
    dfe->Set(Appearance.StateMessageAlligne);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxBorder"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Box"));
    dfe->addEnumText(TEXT("Tab"));
    dfe->Set(Appearance.InfoBoxBorder);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppCompassAppearance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Normal"));
    dfe->addEnumText(TEXT("White outline"));
    dfe->Set(Appearance.CompassAppearance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Default"));
    dfe->addEnumText(TEXT("Alternate"));
    dfe->Set(Appearance.IndFinalGlide);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Winpilot"));
    dfe->addEnumText(TEXT("Alternate"));
    dfe->Set(Appearance.IndLandable);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableExternalTriggerCruise"));
  if (wp) {
    wp->GetDataField()->Set(EnableExternalTriggerCruise);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InverseInfoBox);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppDefaultMapWidth"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(Appearance.DefaultMapWidth);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpGliderScreenPosition"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MapWindow::GliderScreenPosition);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainContrast"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(TerrainContrast*100/255));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainBrightness"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(TerrainBrightness*100/255));
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxColors"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.InfoBoxColors);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioSpeedToFly);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioAvgText"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioAvgText);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioMc"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioMc);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBugs"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioBugs);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBallast"));
  if (wp) {
    wp->GetDataField()->Set(Appearance.GaugeVarioBallast);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->addEnumText(TEXT("FAI Sector"));
    dfe->Set(FinishLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(FinishRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("Line"));
    dfe->addEnumText(TEXT("FAI Sector"));
    dfe->Set(StartLine);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(StartRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Cylinder"));
    dfe->addEnumText(TEXT("FAI Sector"));
    dfe->addEnumText(TEXT("DAe 0.5/10"));
    dfe->Set(SectorType);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(SectorRadius);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    DataFieldEnum* dfe;
    dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->addEnumText(TEXT("Manual"));
    dfe->addEnumText(TEXT("Auto"));
    dfe->addEnumText(TEXT("Arm"));
    dfe->addEnumText(TEXT("Arm start"));
    dfe->Set(AutoAdvance);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinishMinHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(FinishMinHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeight"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxHeight*ALTITUDEMODIFY));
    wp->GetDataField()->SetUnits(Units::GetAltitudeName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeed"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(iround(StartMaxSpeed*SPEEDMODIFY));
    wp->GetDataField()->SetUnits(Units::GetHorizontalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCruise"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(LoggerTimeStepCruise);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCircling"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(LoggerTimeStepCircling);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSnailWidthScale"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(MapWindow::SnailWidthScale);
    wp->RefreshDisplay();
  }

  ////

  SetInfoBoxSelector(TEXT("prpInfoBoxCruise0"),0,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise1"),1,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise2"),2,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise3"),3,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise4"),4,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise5"),5,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise6"),6,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise7"),7,0);
  SetInfoBoxSelector(TEXT("prpInfoBoxCruise8"),8,0);

  SetInfoBoxSelector(TEXT("prpInfoBoxCircling0"),0,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling1"),1,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling2"),2,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling3"),3,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling4"),4,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling5"),5,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling6"),6,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling7"),7,1);
  SetInfoBoxSelector(TEXT("prpInfoBoxCircling8"),8,1);

  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide0"),0,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide1"),1,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide2"),2,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide3"),3,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide4"),4,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide5"),5,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide6"),6,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide7"),7,2);
  SetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide8"),8,2);

  SetInfoBoxSelector(TEXT("prpInfoBoxAux0"),0,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux1"),1,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux2"),2,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux3"),3,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux4"),4,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux5"),5,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux6"),6,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux7"),7,3);
  SetInfoBoxSelector(TEXT("prpInfoBoxAux8"),8,3);

  // TODO: configuration of Appearance

  ////

  NextPage(0); // JMW just to turn proper pages on/off

  changed = false;
  taskchanged = false;
  requirerestart = false;
  utcchanged = false;

  wf->ShowModal();

  // TODO: implement a cancel button that skips all this below after exit.

  wp = (WndProperty*)wf->FindByName(TEXT("prpAbortSafetyUseCurrent"));
  if (wp) {
    if (GlidePolar::AbortSafetyUseCurrent
	!= wp->GetDataField()->GetAsBoolean()) {
      GlidePolar::AbortSafetyUseCurrent =
	wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAbortSafetyUseCurrent,
		    GlidePolar::AbortSafetyUseCurrent);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyMacCready"));
  if (wp) {
    double val = wp->GetDataField()->GetAsFloat()/LIFTMODIFY;
    if (GlidePolar::SafetyMacCready != val) {
      GlidePolar::SafetyMacCready = val;
      SetToRegistry(szRegistrySafetyMacCready,
		    iround(GlidePolar::SafetyMacCready*10));
      changed = true;
    }
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpSetSystemTimeFromGPS"));
  if (wp) {
    if (SetSystemTimeFromGPS != wp->GetDataField()->GetAsBoolean()) {
      SetSystemTimeFromGPS = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistrySetSystemTimeFromGPS, SetSystemTimeFromGPS);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAnimation"));
  if (wp) {
    if (EnableAnimation != wp->GetDataField()->GetAsBoolean()) {
      EnableAnimation = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAnimation, EnableAnimation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrailDrift"));
  if (wp) {
    if (MapWindow::EnableTrailDrift != wp->GetDataField()->GetAsBoolean()) {
      MapWindow::EnableTrailDrift = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryTrailDrift, MapWindow::EnableTrailDrift);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpThermalLocator"));
  if (wp) {
    if (EnableThermalLocator != wp->GetDataField()->GetAsInteger()) {
      EnableThermalLocator = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryThermalLocator, EnableThermalLocator);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTrail"));
  if (wp) {
    if (TrailActive != wp->GetDataField()->GetAsInteger()) {
      TrailActive = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailTrail, TrailActive);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarType"));
  if (wp) {
    if (POLARID != wp->GetDataField()->GetAsInteger()) {
      POLARID = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryPolarID, POLARID);
      GlidePolar::SetBallast();
      POLARFILECHANGED = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceDisplay"));
  if (wp) {
    if (AltitudeMode != wp->GetDataField()->GetAsInteger()) {
      AltitudeMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltMode, AltitudeMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLockSettingsInFlight"));
  if (wp) {
    if (LockSettingsInFlight !=
	wp->GetDataField()->GetAsBoolean()) {
      LockSettingsInFlight = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryLockSettingsInFlight,
		    LockSettingsInFlight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableFLARMDisplay"));
  if (wp) {
    if (EnableFLARMDisplay !=
	wp->GetDataField()->GetAsBoolean()) {
      EnableFLARMDisplay = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryEnableFLARMDisplay,
		    EnableFLARMDisplay);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpDebounceTimeout"));
  if (wp) {
    if (debounceTimeout != wp->GetDataField()->GetAsInteger()) {
      debounceTimeout = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDebounceTimeout, debounceTimeout);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceOutline"));
  if (wp) {
    if (MapWindow::bAirspaceBlackOutline !=
	wp->GetDataField()->GetAsBoolean()) {
      MapWindow::bAirspaceBlackOutline = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAirspaceBlackOutline,
		    MapWindow::bAirspaceBlackOutline);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoZoom"));
  if (wp) {
    if (MapWindow::AutoZoom !=
	wp->GetDataField()->GetAsBoolean()) {
      MapWindow::AutoZoom = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAutoZoom,
		    MapWindow::AutoZoom);
      changed = true;
    }
  }

  int ival;

  wp = (WndProperty*)wf->FindByName(TEXT("prpUTCOffset"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()*3600.0);
    if ((UTCOffset != ival)||(utcchanged)) {
      UTCOffset = ival;

      // have to do this because registry variables can't be negative!
      int lival = UTCOffset;
      if (lival<0) { lival+= 24; }
      SetToRegistry(szRegistryUTCOffset, lival);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpClipAltitude"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (ClipAltitude != ival) {
      ClipAltitude = ival;
      SetToRegistry(szRegistryClipAlt,ClipAltitude);  // fixed 20060430/sgi was szRegistryAltMode
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAltWarningMargin"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (AltWarningMargin != ival) {
      AltWarningMargin = ival;
      SetToRegistry(szRegistryAltMargin,AltWarningMargin);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceWarnings"));
  if (wp) {
    if (AIRSPACEWARNINGS != wp->GetDataField()->GetAsInteger()) {
      AIRSPACEWARNINGS = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAirspaceWarning,(DWORD)AIRSPACEWARNINGS);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWarningTime"));
  if (wp) {
    if (WarningTime != wp->GetDataField()->GetAsInteger()) {
      WarningTime = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAcknowledgementTime"));
  if (wp) {
    if (AcknowledgementTime != wp->GetDataField()->GetAsInteger()) {
      AcknowledgementTime = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAcknowledgementTime,
		    (DWORD)AcknowledgementTime);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointLabels"));
  if (wp) {
    if (DisplayTextType != wp->GetDataField()->GetAsInteger()) {
      DisplayTextType = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDisplayText,DisplayTextType);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTerrain"));
  if (wp) {
    if (EnableTerrain != wp->GetDataField()->GetAsBoolean()) {
      EnableTerrain = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryDrawTerrain, EnableTerrain);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableTopology"));
  if (wp) {
    if (EnableTopology != wp->GetDataField()->GetAsBoolean()) {
      EnableTopology = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryDrawTopology, EnableTopology);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpCirclingZoom"));
  if (wp) {
    if (CircleZoom != wp->GetDataField()->GetAsBoolean()) {
      CircleZoom = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryCircleZoom, CircleZoom);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpOrientation"));
  if (wp) {
    if (DisplayOrientation != wp->GetDataField()->GetAsInteger()) {
      DisplayOrientation = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDisplayUpValue,DisplayOrientation);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMenuTimeout"));
  if (wp) {
    if (MenuTimeoutMax != wp->GetDataField()->GetAsInteger()*2) {
      MenuTimeoutMax = wp->GetDataField()->GetAsInteger()*2;
      SetToRegistry(szRegistryMenuTimeout,MenuTimeoutMax);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeArrival"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDEARRIVAL != ival) {
      SAFETYALTITUDEARRIVAL = ival;
      SetToRegistry(szRegistrySafetyAltitudeArrival,
		    (DWORD)SAFETYALTITUDEARRIVAL);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeBreakoff"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDEBREAKOFF != ival) {
      SAFETYALTITUDEBREAKOFF = ival;
      SetToRegistry(szRegistrySafetyAltitudeBreakOff,
		    (DWORD)SAFETYALTITUDEBREAKOFF);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSafetyAltitudeTerrain"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if (SAFETYALTITUDETERRAIN != ival) {
      SAFETYALTITUDETERRAIN = ival;
      SetToRegistry(szRegistrySafetyAltitudeTerrain,
		    (DWORD)SAFETYALTITUDETERRAIN);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoWind"));
  if (wp) {
    if (AutoWindMode != wp->GetDataField()->GetAsInteger()) {
      AutoWindMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoWind, AutoWindMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoMcMode"));
  if (wp) {
    if (AutoMcMode != wp->GetDataField()->GetAsInteger()) {
      AutoMcMode = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoMcMode, AutoMcMode);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointsOutOfRange"));
  if (wp) {
    if (WaypointsOutOfRange != wp->GetDataField()->GetAsInteger()) {
      WaypointsOutOfRange = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryWaypointsOutOfRange, WaypointsOutOfRange);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoForceFinalGlide"));
  if (wp) {
    if (AutoForceFinalGlide != wp->GetDataField()->GetAsBoolean()) {
      AutoForceFinalGlide = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryAutoForceFinalGlide, AutoForceFinalGlide);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableNavBaroAltitude"));
  if (wp) {
    if (EnableNavBaroAltitude != wp->GetDataField()->GetAsBoolean()) {
      EnableNavBaroAltitude = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryEnableNavBaroAltitude, EnableNavBaroAltitude);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpFinalGlideTerrain"));
  if (wp) {
    if (FinalGlideTerrain != wp->GetDataField()->GetAsBoolean()) {
      FinalGlideTerrain = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryFinalGlideTerrain, FinalGlideTerrain);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpBlockSTF"));
  if (wp) {
    if (EnableBlockSTF != wp->GetDataField()->GetAsBoolean()) {
      EnableBlockSTF = wp->GetDataField()->GetAsBoolean();
      SetToRegistry(szRegistryBlockSTF, EnableBlockSTF);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsSpeed"));
  if (wp) {
    if ((int)Speed != wp->GetDataField()->GetAsInteger()) {
      Speed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySpeedUnitsValue, Speed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsTaskSpeed"));
  if (wp) {
    if ((int)TaskSpeed != wp->GetDataField()->GetAsInteger()) {
      TaskSpeed = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryTaskSpeedUnitsValue, TaskSpeed);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsDistance"));
  if (wp) {
    if ((int)Distance != wp->GetDataField()->GetAsInteger()) {
      Distance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryDistanceUnitsValue, Distance);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsLift"));
  if (wp) {
    if ((int)Lift != wp->GetDataField()->GetAsInteger()) {
      Lift = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryLiftUnitsValue, Lift);
      Units::NotifyUnitChanged();
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpUnitsAltitude"));
  if (wp) {
    if ((int)Altitude != wp->GetDataField()->GetAsInteger()) {
      Altitude = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAltitudeUnitsValue, Altitude);
      Units::NotifyUnitChanged();
      changed = true;
      requirerestart = true;
    }
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpOLCRules"));
  if (wp) {
    if (OLCRules != wp->GetDataField()->GetAsInteger()) {
      OLCRules = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryOLCRules, OLCRules);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpHandicap"));
  if (wp) {
    int val  = wp->GetDataField()->GetAsInteger();
    if (Handicap != val) {
      Handicap = val;
      SetToRegistry(szRegistryHandicap, Handicap);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpPolarFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szPolarFile)) {
      SetRegistryString(szRegistryPolarFile, temptext);
      POLARFILECHANGED = true;
      GlidePolar::SetBallast();
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szWaypointFile)) {
      SetRegistryString(szRegistryWayPointFile, temptext);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalWaypointFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAdditionalWaypointFile)) {
      SetRegistryString(szRegistryAdditionalWayPointFile, temptext);
      WAYPOINTFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAirspaceFile)) {
      SetRegistryString(szRegistryAirspaceFile, temptext);
      AIRSPACEFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAdditionalAirspaceFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAdditionalAirspaceFile)) {
      SetRegistryString(szRegistryAdditionalAirspaceFile, temptext);
      AIRSPACEFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szTerrainFile)) {
      SetRegistryString(szRegistryTerrainFile, temptext);
      TERRAINFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTopologyFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szTopologyFile)) {
      SetRegistryString(szRegistryTopologyFile, temptext);
      TOPOLOGYFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAirfieldFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szAirfieldFile)) {
      SetRegistryString(szRegistryAirfieldFile, temptext);
      AIRFIELDFILECHANGED= true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLanguageFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szLanguageFile)) {
      SetRegistryString(szRegistryLanguageFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStatusFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szStatusFile)) {
      SetRegistryString(szRegistryStatusFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpInputFile"));
  if (wp) {
    DataFieldFileReader* dfe;
    dfe = (DataFieldFileReader*)wp->GetDataField();
    _tcscpy(temptext, dfe->GetPathFile());
    ContractLocalPath(temptext);
    if (_tcscmp(temptext,szInputFile)) {
      SetRegistryString(szRegistryInputFile, temptext);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMaxManoeuveringSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if (SAFTEYSPEED != ival) {
      SAFTEYSPEED = ival;
      SetToRegistry(szRegistrySafteySpeed,(DWORD)SAFTEYSPEED);
      GlidePolar::SetBallast();
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishLine"));
  if (wp) {
    if (FinishLine != wp->GetDataField()->GetAsInteger()) {
      FinishLine = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFinishLine,FinishLine);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFinishRadius"));
  if (wp) {
    if ((int)FinishRadius != wp->GetDataField()->GetAsInteger()) {
      FinishRadius = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFinishRadius,FinishRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartLine"));
  if (wp) {
    if (StartLine != wp->GetDataField()->GetAsInteger()) {
      StartLine = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartLine,StartLine);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskStartRadius"));
  if (wp) {
    if ((int)StartRadius != wp->GetDataField()->GetAsInteger()) {
      StartRadius = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryStartRadius,StartRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskFAISector"));
  if (wp) {
    if ((int)SectorType != wp->GetDataField()->GetAsInteger()) {
      SectorType = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryFAISector,SectorType);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTaskSectorRadius"));
  if (wp) {
    if ((int)SectorRadius != wp->GetDataField()->GetAsInteger()) {
      SectorRadius = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySectorRadius,SectorRadius);
      changed = true;
      taskchanged = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndFinalGlide"));
  if (wp) {
    if (Appearance.IndFinalGlide != (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndFinalGlide = (IndFinalGlide_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndFinalGlide,(DWORD)(Appearance.IndFinalGlide));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppCompassAppearance"));
  if (wp) {
    if (Appearance.CompassAppearance != (CompassAppearance_t)
	(wp->GetDataField()->GetAsInteger())) {
      Appearance.CompassAppearance = (CompassAppearance_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppCompassAppearance,
		    (DWORD)(Appearance.CompassAppearance));
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxBorder"));
  if (wp) {
    if (Appearance.InfoBoxBorder != (InfoBoxBorderAppearance_t)
	(wp->GetDataField()->GetAsInteger())) {
      Appearance.InfoBoxBorder = (InfoBoxBorderAppearance_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppInfoBoxBorder,
		    (DWORD)(Appearance.InfoBoxBorder));
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppStatusMessageAlignment"));
  if (wp) {
    if (Appearance.StateMessageAlligne != (StateMessageAlligne_t)
	(wp->GetDataField()->GetAsInteger())) {
      Appearance.StateMessageAlligne = (StateMessageAlligne_t)
	(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppStatusMessageAlignment,
		    (DWORD)(Appearance.StateMessageAlligne));
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppIndLandable"));
  if (wp) {
    if (Appearance.IndLandable != (IndLandable_t)(wp->GetDataField()->GetAsInteger())) {
      Appearance.IndLandable = (IndLandable_t)(wp->GetDataField()->GetAsInteger());
      SetToRegistry(szRegistryAppIndLandable,(DWORD)(Appearance.IndLandable));
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEnableExternalTriggerCruise"));
  if (wp) {
    if ((int)(EnableExternalTriggerCruise) !=
	wp->GetDataField()->GetAsInteger()) {
      EnableExternalTriggerCruise = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryEnableExternalTriggerCruise,
		    EnableExternalTriggerCruise);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInverseInfoBox"));
  if (wp) {
    if ((int)(Appearance.InverseInfoBox) != wp->GetDataField()->GetAsInteger()) {
      Appearance.InverseInfoBox = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInverseInfoBox,Appearance.InverseInfoBox);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpGliderScreenPosition"));
  if (wp) {
    if (MapWindow::GliderScreenPosition !=
	wp->GetDataField()->GetAsInteger()) {
      MapWindow::GliderScreenPosition = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryGliderScreenPosition,
		    MapWindow::GliderScreenPosition);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppDefaultMapWidth"));
  if (wp) {
    if ((int)(Appearance.DefaultMapWidth) !=
	wp->GetDataField()->GetAsInteger()) {
      Appearance.DefaultMapWidth = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAppDefaultMapWidth,Appearance.DefaultMapWidth);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppInfoBoxColors"));
  if (wp) {
    if ((int)(Appearance.InfoBoxColors) !=
	wp->GetDataField()->GetAsInteger()) {
      Appearance.InfoBoxColors = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppInfoBoxColors,Appearance.InfoBoxColors);
      requirerestart = true;
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioSpeedToFly"));
  if (wp) {
    if ((int)(Appearance.GaugeVarioSpeedToFly) != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioSpeedToFly = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioSpeedToFly,Appearance.GaugeVarioSpeedToFly);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioAvgText"));
  if (wp) {
    if ((int)Appearance.GaugeVarioAvgText != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioAvgText = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioAvgText,Appearance.GaugeVarioAvgText);
      changed = true;
      requirerestart = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioMc"));
  if (wp) {
    if ((int)Appearance.GaugeVarioMc != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioMc = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioMc,Appearance.GaugeVarioMc);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBugs"));
  if (wp) {
    if ((int)Appearance.GaugeVarioBugs != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBugs = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioBugs,Appearance.GaugeVarioBugs);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpAppGaugeVarioBallast"));
  if (wp) {
    if ((int)Appearance.GaugeVarioBallast != wp->GetDataField()->GetAsInteger()) {
      Appearance.GaugeVarioBallast = (wp->GetDataField()->GetAsInteger() != 0);
      SetToRegistry(szRegistryAppGaugeVarioBallast,Appearance.GaugeVarioBallast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainContrast"));
  if (wp) {
    if (iround(TerrainContrast*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainContrast = iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainContrast,TerrainContrast);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpTerrainBrightness"));
  if (wp) {
    if (iround(TerrainBrightness*100/255) !=
	wp->GetDataField()->GetAsInteger()) {
      TerrainBrightness = iround(wp->GetDataField()->GetAsInteger()*255.0/100);
      SetToRegistry(szRegistryTerrainBrightness,TerrainBrightness);
      changed = true;
    }
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpFinishMinHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)FinishMinHeight != ival) {
      FinishMinHeight = ival;
      SetToRegistry(szRegistryFinishMinHeight,FinishMinHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxHeight"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/ALTITUDEMODIFY);
    if ((int)StartMaxHeight != ival) {
      StartMaxHeight = ival;
      SetToRegistry(szRegistryStartMaxHeight,StartMaxHeight);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpStartMaxSpeed"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger()/SPEEDMODIFY);
    if ((int)StartMaxSpeed != ival) {
      StartMaxSpeed = ival;
      SetToRegistry(szRegistryStartMaxSpeed,StartMaxSpeed);
      changed = true;
    }
  }


  wp = (WndProperty*)wf->FindByName(TEXT("prpAutoAdvance"));
  if (wp) {
    if (AutoAdvance != wp->GetDataField()->GetAsInteger()) {
      AutoAdvance = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistryAutoAdvance,
		    AutoAdvance);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCruise"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger());
    if (LoggerTimeStepCruise != ival) {
      LoggerTimeStepCruise = ival;
      SetToRegistry(szRegistryLoggerTimeStepCruise,LoggerTimeStepCruise);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpLoggerTimeStepCircling"));
  if (wp) {
    ival = iround(wp->GetDataField()->GetAsInteger());
    if (LoggerTimeStepCircling != ival) {
      LoggerTimeStepCircling = ival;
      SetToRegistry(szRegistryLoggerTimeStepCircling,LoggerTimeStepCircling);
      changed = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort1"));
  if (wp) {
    if ((int)dwPortIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed1"));
  if (wp) {
    if ((int)dwSpeedIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice1"));
  if (wp) {
    if (dwDeviceIndex1 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex1 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex1-1, DeviceName);
      WriteDeviceSettings(0, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComPort2"));
  if (wp) {
    if ((int)dwPortIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwPortIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComSpeed2"));
  if (wp) {
    if ((int)dwSpeedIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwSpeedIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpComDevice2"));
  if (wp) {
    if (dwDeviceIndex2 != wp->GetDataField()->GetAsInteger()) {
      dwDeviceIndex2 = wp->GetDataField()->GetAsInteger();
      changed = true;
      COMPORTCHANGED = true;
      devRegisterGetName(dwDeviceIndex2-1, DeviceName);
      WriteDeviceSettings(1, DeviceName);
    }
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSnailWidthScale"));
  if (wp) {
    if (MapWindow::SnailWidthScale != wp->GetDataField()->GetAsInteger()) {
      MapWindow::SnailWidthScale = wp->GetDataField()->GetAsInteger();
      SetToRegistry(szRegistrySnailWidthScale,MapWindow::SnailWidthScale);
      changed = true;
      requirerestart = true;
    }
  }

  if (COMPORTCHANGED) {
    WritePort1Settings(dwPortIndex1,dwSpeedIndex1);
    WritePort2Settings(dwPortIndex2,dwSpeedIndex2);
  }

  GetInfoBoxSelector(TEXT("prpInfoBoxCruise0"),0,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise1"),1,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise2"),2,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise3"),3,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise4"),4,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise5"),5,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise6"),6,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise7"),7,0);
  GetInfoBoxSelector(TEXT("prpInfoBoxCruise8"),8,0);

  GetInfoBoxSelector(TEXT("prpInfoBoxCircling0"),0,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling1"),1,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling2"),2,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling3"),3,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling4"),4,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling5"),5,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling6"),6,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling7"),7,1);
  GetInfoBoxSelector(TEXT("prpInfoBoxCircling8"),8,1);

  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide0"),0,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide1"),1,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide2"),2,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide3"),3,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide4"),4,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide5"),5,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide6"),6,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide7"),7,2);
  GetInfoBoxSelector(TEXT("prpInfoBoxFinalGlide8"),8,2);

  GetInfoBoxSelector(TEXT("prpInfoBoxAux0"),0,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux1"),1,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux2"),2,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux3"),3,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux4"),4,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux5"),5,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux6"),6,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux7"),7,3);
  GetInfoBoxSelector(TEXT("prpInfoBoxAux8"),8,3);

  if (taskchanged) {
    RefreshTask();
  }

#if (WINDOWSPC>0)
  if (COMPORTCHANGED) {
    requirerestart = true;
  }
#endif

  if (changed) {
    StoreRegistry();

    if (!requirerestart) {
      MessageBoxX (hWndMainWindow,
		   gettext(TEXT("Changes to configuration saved.")),
		   TEXT(""), MB_OK);
    } else {

      MessageBoxX (hWndMainWindow,
		   gettext(TEXT("Changes to configuration saved.  Restart XCSoar to apply changes.")),
		   TEXT(""), MB_OK);
    }
  }

  delete wf;

  wf = NULL;

}

// extern TCHAR szRegistryAirspaceBlackOutline[];
// NUMAIRSPACECOLORS
// AIRSPACECLASSCOUNT
// SetRegistryColour(CTR, NewColor);
/*
 AIRSPACECLASSCOUNT

  { CLASSA,     _T("Class A")},
  { CLASSB,     _T("Class B")},
  { CLASSC,     _T("Class C")},
  { CLASSD,     _T("Class D")},
  { CLASSE,     _T("Class E")},
  { CLASSF,     _T("Class F")},
  { PROHIBITED, _T("Prohibited areas")},
  { DANGER,     _T("Danger areas")},
  { RESTRICT,   _T("Restricted areas")},
  { CTR,        _T("CTR")},
  { NOGLIDER,   _T("No Gliders")},
  { WAVE,       _T("Wave")},
  { OTHER,      _T("Other")},
  { AATASK,     _T("AAT")}
*/

