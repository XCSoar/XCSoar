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
#include "LoggerImpl.hpp"
#include "Asset.hpp"
#include "wcecompat/ts_string.h"
#include <tchar.h>
#include <stdio.h>


/*
 * From FAI_Tech_Spec_Gnss.pdf 
 * 4.3 F RECORD - SATELLITE CONSTELLATION.
 * This is a mandatory record. However, there is no requirement to update the F-record at intervals of less than 5
 * minutes, so that transient changes of satellites received due to changing angles of bank, flying in valleys, etc do
 * not lead to frequent F-record lines. For the US GPS system, the satellite ID for each satellite is the PRN of the
 * satellite in question, for other satellite systems the ID will be assigned by GFAC as the need arises. Where
 * NMEA data is used within the FR, the ID should be taken from the GSA sentence that lists the IDs of those
 * satellites used in the fixes which are recorded in the B record. The F Record is not recorded continuously but at
 * the start of fixing and then only when a change in satellites used is detected. (AL4)
 */
/* Interpretation:
 * Every logpoint, check if constellation has changed, and set flag if change is detected
 * every 4.5 minutes, if constellation has changed during the period
 * then log the new FRecord
 * Else, don't log it
 * Note: if a NAV Warning exists, we accelerate checking to every 30 seconds for valid constellation.
 * This is not required, but seems advantageous
 */

void
LoggerImpl::ResetFRecord(void)
{
  LastFRecordValid=true;
  DetectFRecordChange=true;
  frecord_clock.reset(); // reset clock / timer
  frecord_clock.set_dt(1); // 1 sec so it appears at top of each file
  for (int iFirst = 0; iFirst < MAX_IGC_BUFF; iFirst++)
    szLastFRecord[iFirst]=0;
}
void
LoggerImpl::LogFRecordToFile(const int SatelliteIDs[],
                         short Hour, 
                         short Minute, 
                         short Second, 
                         double Time,
                         int NAVWarning)
{ 
  if (is_simulator())
    return;

  char szFRecord[MAX_IGC_BUFF];
  int eof=0;
  int iNumberSatellites=0;
  
  sprintf(szFRecord,"F%02d%02d%02d", Hour, Minute, Second);
  eof=7;

  for (int i=0; i < MAXSATELLITES; i++){
    if (SatelliteIDs[i] > 0){
      sprintf(szFRecord+eof, "%02d",SatelliteIDs[i]);
      eof +=2;
      iNumberSatellites++;
    }
  }
  sprintf(szFRecord+ eof,"\r\n");

  if (strcmp(szFRecord + 7, szLastFRecord + 7) != 0 ||
       strlen(szFRecord) != strlen(szLastFRecord) ) {
    DetectFRecordChange=true;
  }
  
  if (iNumberSatellites < 3 || NAVWarning) {
    frecord_clock.set_dt(30);  // accelerate to 30 seconds if bad signal
  }
   
  if ( frecord_clock.check_advance(Time) && DetectFRecordChange) {
    if (IGCWriteRecord(szFRecord, szLoggerFileName)) {
      strcpy(szLastFRecord, szFRecord);
      DetectFRecordChange=false;
      frecord_clock.set_dt(270); //4.5 minutes
    }
  }

  return;
}
