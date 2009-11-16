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


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "Device/devVolkslogger.h"
#include "Device/Internal.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "Interface.hpp"
#include "Language.hpp"
#include "Task.h"
#include "UtilsText.hpp"
#include "Math/Pressure.h"
#include "Device/Parser.h"
#include "Device/Port.h"
#include "WayPointList.hpp"
#include "Components.hpp"
#include "Device/Volkslogger/vlapi2.h"
#include "Device/Volkslogger/vlapihlp.h"
#include "Components.hpp"
#include "WayPointList.hpp"

class VolksloggerDevice : public AbstractDevice {
private:
  ComPort *port;

public:
  VolksloggerDevice(ComPort *_port):port(_port) {}

public:
  virtual bool ParseNMEA(const TCHAR *line, struct NMEA_INFO *info,
                         bool enable_baro);
  virtual bool Declare(const struct Declaration *declaration);
};

// RMN: Volkslogger
// Source data:
// $PGCS,1,0EC0,FFF9,0C6E,02*61
// $PGCS,1,0EC0,FFFA,0C6E,03*18
static bool
vl_PGCS1(const TCHAR *String, NMEA_INFO *GPS_INFO, bool enable_baro)
{

  TCHAR ctemp[80];
  double InternalAltitude;

  NMEAParser::ExtractParameter(String,ctemp,2);
  // four characers, hex, barometric altitude
  InternalAltitude = HexStrToDouble(ctemp,NULL);

  if (enable_baro) {
    if(InternalAltitude > 60000)
      GPS_INFO->BaroAltitude =
        AltitudeToQNHAltitude(InternalAltitude - 65535);
    // Assuming that altitude has wrapped around.  60 000 m occurs at
    // QNH ~2000 hPa
    else
      GPS_INFO->BaroAltitude =
        AltitudeToQNHAltitude(InternalAltitude);
    // typo corrected 21.04.07
    // Else the altitude is good enough.

    GPS_INFO->BaroAltitudeAvailable = true;
  }

  // ExtractParameter(String,ctemp,3);
  // four characters, hex, constant.  Value 1371 (dec)

  // nSatellites = (int)(min(12,HexStrToDouble(ctemp, NULL)));

  if (GPS_INFO->SatellitesUsed <= 0) {
    GPS_INFO->SatellitesUsed = 4;
    // just to make XCSoar quit complaining.  VL doesn't tell how many
    // satellites it uses.  Without this XCSoar won't do wind
    // measurements.
  }

  return false;
}

bool
VolksloggerDevice::ParseNMEA(const TCHAR *String, NMEA_INFO *GPS_INFO,
                             bool enable_baro)
{
  if (!NMEAParser::NMEAChecksum(String) || (GPS_INFO == NULL)){
    return false;
  }

  if(_tcsstr(String, _T("$PGCS,")) == String){
    return vl_PGCS1(&String[6], GPS_INFO, enable_baro);
  }

  return false;

}

static VLAPI vl;

static int nturnpoints = 0;

static bool
VLDeclAddWayPoint(const WAYPOINT *wp);

bool
VolksloggerDevice::Declare(const struct Declaration *decl)
{
  XCSoarInterface::CreateProgressDialog(gettext(_T("Comms with Volkslogger")));

  vl.set_port(port);
  nturnpoints = 0;

  int err;
  if((err = vl.open(1, 20, 1, 38400L)) != VLA_ERR_NOERR) {
    //    _isConnected = false;
  }
  else {
    //    _isConnected = true;
  }

  if (err == VLA_ERR_NOERR) {
    if ((err = vl.read_info()) == VLA_ERR_NOERR) {
      //      vl.read_db_and_declaration();
    }

    char temp[100];
    sprintf(temp, "%S", decl->PilotName);
    strncpy(vl.declaration.flightinfo.pilot, temp, 64);

    sprintf(temp, "%S", decl->AircraftRego);
    strncpy(vl.declaration.flightinfo.competitionid, temp, 3);

    sprintf(temp, "%S", decl->AircraftType);
    strncpy(vl.declaration.flightinfo.competitionclass, temp, 12);

    if (way_points.verify_index(XCSoarInterface::SettingsComputer().HomeWaypoint)) {
      const WAYPOINT &way_point = way_points.get(XCSoarInterface::SettingsComputer().HomeWaypoint);

      sprintf(temp, "%S", way_point.Name);

      strncpy(vl.declaration.flightinfo.homepoint.name, temp, 6);
      vl.declaration.flightinfo.homepoint.lon = way_point.Location.Longitude;
      vl.declaration.flightinfo.homepoint.lat = way_point.Location.Latitude;
    }
  }

  if (err != VLA_ERR_NOERR)
    return false;

  int i;
  for (i = 0; i < decl->num_waypoints; i++)
    VLDeclAddWayPoint(decl->waypoint[i]);

  vl.declaration.task.nturnpoints = max(min(nturnpoints-2, 12), 0);

  const SETTINGS_TASK settings = task.getSettings();

  // start..
  switch(settings.StartType) {
  case START_CIRCLE:
    vl.declaration.task.startpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    vl.declaration.task.startpoint.lw = min(1500,settings.StartRadius);
    vl.declaration.task.startpoint.rz = min(1500,settings.StartRadius);
    vl.declaration.task.startpoint.rs = 0;
    break;
  case START_LINE:
    vl.declaration.task.startpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_LINE;
    vl.declaration.task.startpoint.lw = min(1500,settings.StartRadius*2);
    vl.declaration.task.startpoint.rs = 0;
    vl.declaration.task.startpoint.rz = 0;
    break;
  case START_SECTOR:
    vl.declaration.task.startpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    vl.declaration.task.startpoint.lw = min(1500,settings.StartRadius);
    vl.declaration.task.startpoint.rz = 0;
    vl.declaration.task.startpoint.rs = min(1500,settings.StartRadius);
    break;
  }
  vl.declaration.task.startpoint.ws = 360;

  // rest of task...
  for (i=0; i<nturnpoints; i++) {
    // note this is for non-aat only!
    switch (settings.SectorType) {
    case 0: // cylinder
      vl.declaration.task.turnpoints[i].oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
      vl.declaration.task.turnpoints[i].rz = settings.SectorRadius;
      vl.declaration.task.turnpoints[i].rs = 0;
      vl.declaration.task.turnpoints[i].lw = 0;
      break;
    case 1: // sector
      vl.declaration.task.turnpoints[i].oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
      vl.declaration.task.turnpoints[i].rz = 0;
      vl.declaration.task.turnpoints[i].rs = 15000;
      vl.declaration.task.turnpoints[i].lw = 0;
      break;
    case 2: // German DAe 0.5/10
      vl.declaration.task.turnpoints[i].oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
      vl.declaration.task.turnpoints[i].rz = 500;
      vl.declaration.task.turnpoints[i].rs = 10000;
      vl.declaration.task.turnpoints[i].lw = 0;
      break;
    };
    vl.declaration.task.turnpoints[i].ws = 360; // auto direction

  }

  // Finish
  switch(settings.FinishType) {
  case FINISH_CIRCLE:
    vl.declaration.task.finishpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    vl.declaration.task.finishpoint.lw = min(1500,settings.FinishRadius);
    vl.declaration.task.finishpoint.rz = min(1500,settings.FinishRadius);
    vl.declaration.task.finishpoint.rs = 0;
    break;
  case FINISH_LINE:
    vl.declaration.task.finishpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_LINE;
    vl.declaration.task.finishpoint.lw = settings.FinishRadius*2;
    vl.declaration.task.finishpoint.rz = 0;
    vl.declaration.task.finishpoint.rs = 0;
    break;
  case FINISH_SECTOR:
    vl.declaration.task.finishpoint.oztyp = VLAPI_DATA::DCLWPT::OZTYP_CYLSKT;
    vl.declaration.task.finishpoint.lw = min(1500,settings.FinishRadius);
    vl.declaration.task.finishpoint.rz = 0;
    vl.declaration.task.finishpoint.rs = min(1500,settings.FinishRadius);
    break;
  }
  vl.declaration.task.finishpoint.ws = 360;

  bool ok = (vl.write_db_and_declaration() == VLA_ERR_NOERR);

  vl.close(1);

  XCSoarInterface::CloseProgressDialog();

  return ok;
}

static bool
VLDeclAddWayPoint(const WAYPOINT *wp)
{
  char temp[100];
  sprintf(temp, "%S", wp->Name);

  if (nturnpoints == 0) {
    strncpy(vl.declaration.task.startpoint.name, temp, 6);
    vl.declaration.task.startpoint.lon =
      wp->Location.Longitude;
    vl.declaration.task.startpoint.lat =
      wp->Location.Latitude;
    nturnpoints++;
  } else {
    strncpy(vl.declaration.task.turnpoints[nturnpoints-1].name, temp, 6);
    vl.declaration.task.turnpoints[nturnpoints-1].lon =
      wp->Location.Longitude;
    vl.declaration.task.turnpoints[nturnpoints-1].lat =
      wp->Location.Latitude;
    nturnpoints++;
  }
  strncpy(vl.declaration.task.finishpoint.name, temp, 6);
  vl.declaration.task.finishpoint.lon =
    wp->Location.Longitude;
  vl.declaration.task.finishpoint.lat =
    wp->Location.Latitude;

  return true;

}

static Device *
VolksloggerCreateOnComPort(ComPort *com_port)
{
  return new VolksloggerDevice(com_port);
}

const struct DeviceRegister vlDevice = {
  _T("Device/Volkslogger"),
  drfGPS | drfLogger, /* XXX: drfBaroAlt? */
  VolksloggerCreateOnComPort,
};
