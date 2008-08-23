// $Id: devCaiGpsNav.cpp,v 1.6 2008/08/23 06:28:44 jwharington Exp $

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

//


// CAUTION!
// caiGpsNavParseNMEA is called from com port read thread
// all other functions are called from windows message loop thread


#define  LOGSTREAM 0


#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devCaiGpsNav.h"

#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif


#define  CtrlC  0x03
#define  swap(x)      x = ((((x<<8) & 0xff00) | ((x>>8) & 0x00ff)) & 0xffff)


BOOL caiGpsNavOpen(PDeviceDescriptor_t d, int Port){

  if (!fSimMode){
	  d->Com->WriteString(TEXT("\x03"));
	  Sleep(50);
	  d->Com->WriteString(TEXT("NMEA\r"));
	  
	  // This is for a slightly different mode, that
	  // apparently outputs pressure info too...
	  //(d->Com.WriteString)(TEXT("PNP\r\n"));
	  //(d->Com.WriteString)(TEXT("LOG 0\r\n"));
  }
  
  return(TRUE);
}

BOOL caiGpsNavClose(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}




BOOL caiGpsNavIsLogger(PDeviceDescriptor_t d){
  // There is currently no support for task declaration
  // from XCSoar
	(void)d; // TODO
  return(FALSE);
}


BOOL caiGpsNavIsGPSSource(PDeviceDescriptor_t d){
	(void)d;
  return(TRUE);
}


BOOL caiGpsNavInstall(PDeviceDescriptor_t d){

  _tcscpy(d->Name, TEXT("CAI GPS-NAV"));
  d->ParseNMEA = NULL;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = caiGpsNavOpen;
  d->Close = caiGpsNavClose;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->DeclBegin = NULL;
  d->DeclEnd = NULL;
  d->DeclAddWayPoint = NULL;
  d->IsLogger = caiGpsNavIsLogger;
  d->IsGPSSource = caiGpsNavIsGPSSource;

  return(TRUE);

}


BOOL caiGpsNavRegister(void){
  return(devRegister(
    TEXT("CAI GPS-NAV"), 
    (1l << dfGPS),
    caiGpsNavInstall
  ));
}

