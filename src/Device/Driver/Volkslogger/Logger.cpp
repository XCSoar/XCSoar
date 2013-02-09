/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Internal.hpp"
#include "Device/Port/Port.hpp"
#include "Operation/Operation.hpp"
#include "vlapi2.h"


#ifdef _UNICODE
#include <windows.h>
#endif

#include <algorithm>

static bool
ParseDate(tm const &_time,BrokenDate &_date)
{
  _date.year = _time.tm_year + 1900;
  _date.month = _time.tm_mon + 1;
  _date.day = _time.tm_mday;

  return _date.Plausible();

}



static bool
ParseTime(tm const &_time,BrokenTime &_btime)
{
  _btime.hour = _time.tm_hour;
  _btime.minute = _time.tm_min;
  _btime.second = _time.tm_sec;

  return true;

}

static bool
ConvertDirectoryToRecordedFlightList(DIRECTORY const &dir,
                                     RecordedFlightList &flight_list)
{
  RecordedFlightInfo flight_info;
  DIRENTRY *pflight = dir.flights;
  for (uint8_t i=0; i < dir.nflights;i++)
  {
    /*
     * Only show logs with a takeoff detected
     */
    if  (pflight->takeoff == 1)
    {

      if (!ParseDate(pflight->firsttime, flight_info.date) ||
          !ParseTime(pflight->firsttime, flight_info.start_time) ||
          !ParseTime(pflight->lasttime, flight_info.end_time) )
        return false;
      flight_info.internal.volkslogger = i;

      if ( !flight_list.full() )
        flight_list.append(flight_info);
    }

    pflight++;
  }
  return true;
}


static bool
ReadFlightListInner(Port &port, RecordedFlightList &flight_list,
                    OperationEnvironment &env)
{


  VLAPI vl(port, 115200L, env);

  env.SetProgressRange(10);

  if (vl.connect(20) != VLA_ERR_NOERR)
    return false;
  env.SetProgressPosition(3);

  if (vl.read_directory() != VLA_ERR_NOERR)
    return false;

  env.SetProgressPosition(8);

   if (!ConvertDirectoryToRecordedFlightList(vl.directory,flight_list))
    return false;

  env.SetProgressPosition(10);


  return true;
}

static bool
DownloadFlightInner(Port &port,
                    const RecordedFlightInfo &flight,
                    const TCHAR *path,
                    OperationEnvironment &env)
{
  VLAPI vl(port, 115200L, env);

  if (vl.connect(4) != VLA_ERR_NOERR && vl.connect(20) != VLA_ERR_NOERR)
      return false;


  if (vl.read_igcfile(path,flight.internal.volkslogger,1) != VLA_ERR_NOERR)
    return false;

  return true;
}



bool VolksloggerDevice::ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env) {

  port.StopRxThread();

  // change to IO mode baud rate
  unsigned old_baud_rate = port.GetBaudrate();


  if (old_baud_rate == 9600)
    old_baud_rate = 0;
  else if (old_baud_rate != 0 && !port.SetBaudrate(9600))
    return false;

  bool success = ReadFlightListInner(port, flight_list, env);

  // restore baudrate
  if (old_baud_rate != 0)
    port.SetBaudrate(old_baud_rate);

    return success;
}


bool VolksloggerDevice::DownloadFlight(const RecordedFlightInfo &flight,
                              const TCHAR *path,
                              OperationEnvironment &env)
{
  port.StopRxThread();

   // change to IO mode baud rate
  unsigned old_baud_rate = port.GetBaudrate();


  if (old_baud_rate == 9600)
    old_baud_rate = 0;
  else if (old_baud_rate != 0 && !port.SetBaudrate(9600))
    return false;


  bool success = DownloadFlightInner(port,flight,path,env);
   // restore baudrate
  if (old_baud_rate != 0)
     port.SetBaudrate(old_baud_rate);


  return success;

}
