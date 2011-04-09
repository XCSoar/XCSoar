/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Logger/LoggerFRecord.hpp"
#include "DateTime.hpp"
#include "NMEA/Info.hpp"

#include <stdio.h>
#include <string.h>

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
LoggerFRecord::reset()
{
  szLastFRecord[0] = 0;
  DetectFRecordChange=true;
  frecord_clock.reset(); // reset clock / timer
  frecord_clock.set_dt(fixed_one); // 1 sec so it appears at top of each file
}

const char *
LoggerFRecord::update(const int SatelliteIDs[],
                      const BrokenTime &broken_time, fixed Time,
                      bool NAVWarning)
{ 
  char szFRecord[sizeof(szLastFRecord)];
  int eof=0;
  int iNumberSatellites=0;
  
  sprintf(szFRecord,"F%02u%02u%02u",
          broken_time.hour, broken_time.minute, broken_time.second);
  eof=7;

  for (unsigned i = 0; i < GPS_STATE::MAXSATELLITES; ++i) {
    if (SatelliteIDs[i] > 0){
      sprintf(szFRecord+eof, "%02d",SatelliteIDs[i]);
      eof +=2;
      iNumberSatellites++;
    }
  }
  sprintf(szFRecord+ eof,"\r\n");

  DetectFRecordChange = DetectFRecordChange ||
    strcmp(szFRecord + 7, szLastFRecord + 7);
  
  if (iNumberSatellites < 3 || NAVWarning) {
    frecord_clock.set_dt(fixed(30)); // accelerate to 30 seconds if bad signal
  }
   
  if (!frecord_clock.check_advance(fixed(Time)) || !DetectFRecordChange)
    return NULL;

  strcpy(szLastFRecord, szFRecord);
  DetectFRecordChange=false;
  frecord_clock.set_dt(fixed(270)); //4.5 minutes
  return szLastFRecord;
}
