/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

#include "StdAfx.h"

#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devXCOM760.h"


static BOOL XCOM760IsRadio(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


static BOOL XCOM760PutVolume(PDeviceDescriptor_t d, int Volume) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$RVOL=%d\r\n"), Volume);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static BOOL XCOM760PutFreqActive(PDeviceDescriptor_t d, double Freq) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$TXAF=%.3f\r\n"), Freq);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static BOOL XCOM760PutFreqStandby(PDeviceDescriptor_t d, double Freq) {
  TCHAR  szTmp[32];
  _stprintf(szTmp, TEXT("$TXSF=%.3f\r\n"), Freq);
  d->Com->WriteString(szTmp);
  return(TRUE);
}


static BOOL XCOM760Install(PDeviceDescriptor_t d){

  d->IsRadio = XCOM760IsRadio;
  d->PutVolume = XCOM760PutVolume;
  d->PutFreqActive = XCOM760PutFreqActive;
  d->PutFreqStandby = XCOM760PutFreqStandby;
  return(TRUE);

}


BOOL xcom760Register(void){
  return(devRegister(
    TEXT("XCOM760"),
    (1l << dfRadio),
    XCOM760Install
  ));
}


/* Commands

  $TOGG: return to main screen or toggle active and standby
  $DUAL=on/off
*/
