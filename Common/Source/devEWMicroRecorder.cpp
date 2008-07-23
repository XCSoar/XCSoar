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


// ToDo

// adding baro alt sentance parser to support baro source priority  if (d == pDevPrimaryBaroSource){...}

#include "stdafx.h"


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devEWMicroRecorder.h"


// Additional sentance for EW support

static BOOL fDeclarationPending = FALSE;
static unsigned long lLastBaudrate = 0;
static int nDeclErrorCode = 0;
static int ewDecelTpIndex = 0;
static WAYPOINT *wpLast = NULL;
#define MAX_USER_SIZE 2500
static int user_size = 0;
static TCHAR user_data[MAX_USER_SIZE];

#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif


BOOL ExpectStringWait(PDeviceDescriptor_t d, TCHAR *token) {

  int i=0, ch;
  int j=0;

  if (!(d->Com.GetChar)) {
    return (FALSE);
  }

  while (j<500) {

    ch = (d->Com.GetChar)();

    if (ch != EOF) {

      if (token[i] == ch) 
        i++;
      else
        i=0;

      if ((unsigned)i == _tcslen(token))
        return(TRUE);
    }
    j++;
  }

  #if debugIGNORERESPONCE > 0
  return(TRUE);
  #endif
  return(FALSE);

}



BOOL EWMicroRecorderParseNMEA(PDeviceDescriptor_t d, 
                              TCHAR *String, NMEA_INFO *GPS_INFO){
  (void)d;
  (void)String;
  (void)GPS_INFO;
  // no propriatary sentence
  
  return FALSE;

}


BOOL EWMicroRecorderOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  return(TRUE);
}


BOOL EWMicroRecorderTryConnect(PDeviceDescriptor_t d) {
  int retries=10;
  TCHAR ch;

  if (!(d->Com.GetChar)) {
    return (FALSE);
  }

  while (--retries){

    (d->Com.WriteString)(TEXT("\x02"));         // send IO Mode command

    user_size = 0;
    bool started = false;

    while ((ch = (d->Com.GetChar)()) != EOF) {
      if (!started) {
        if (ch == _T('-')) {
          started = true;
        }
      }
      if (started) {
        if (ch == 0x13) {
          (d->Com.WriteString)(TEXT("\x16"));
          user_data[user_size] = 0;
          // found end of file
          return TRUE;
        } else {
          if (user_size<MAX_USER_SIZE-1) {
            user_data[user_size] = ch;
            user_size++;
          }
        }
      }
    }

  }

  nDeclErrorCode = 1;
  return(FALSE);
}


BOOL EWMicroRecorderDeclBegin(PDeviceDescriptor_t d, 
                              TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  //  TCHAR sTmp[72];

  nDeclErrorCode = 0;
  ewDecelTpIndex = 0;
  fDeclarationPending = TRUE;

  if (fSimMode) 
    return(TRUE);

  (d->Com.StopRxThread)();

  (d->Com.SetRxTimeout)(500);                     // set RX timeout to 500[ms]

  if (!EWMicroRecorderTryConnect(d)) {
    return FALSE;
  }

  (d->Com.WriteString)(TEXT("\x18"));         // start to upload file

  TCHAR *ptr;
  ptr = _tcsstr(user_data, TEXT("Description:      Declaration"));
  if (ptr) {
    *ptr = 0;
  }

  (d->Com.WriteString)(user_data);
  (d->Com.WriteString)(TEXT("Description:      Declaration\r\n"));

  wpLast= NULL;

  return(TRUE);

}



static void EWMicroRecorderWriteWayPoint(PDeviceDescriptor_t d, 
                                         WAYPOINT *wp,
                                         TCHAR* EWType) {
  TCHAR EWRecord[128];
  int DegLat, DegLon;
  double MinLat, MinLon;
  TCHAR NoS, EoW;

  DegLat = (int)wp->Latitude;                    // prepare lat
  MinLat = wp->Latitude - DegLat;
  NoS = _T('N');
  if(MinLat<0)
    {
      NoS = _T('S');
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;

  DegLon = (int)wp->Longitude ;                  // prepare long
  MinLon = wp->Longitude  - DegLon;
  EoW = _T('E');
  if(MinLon<0)
    {
      EoW = _T('W');
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *= 60;
  MinLon *= 1000;

  _stprintf(EWRecord,
            TEXT("%s%02d%05d%c%03d%05d%c %s\r\n"),
            EWType,
            DegLat, (int)MinLat, NoS, 
            DegLon, (int)MinLon, EoW, 
            wp->Name);
  if (!fSimMode){
    (d->Com.WriteString)(EWRecord);                 // put it to the logger
  }
}


BOOL EWMicroRecorderDeclEnd(PDeviceDescriptor_t d){

  if (wpLast) {
    TCHAR EWRecord[100];
    for (int i=ewDecelTpIndex; i<12; i++) {
      _stprintf(EWRecord,
                TEXT("TP LatLon:        0000000N00000000E TURN POINT\r\n"));
      if (!fSimMode){
        (d->Com.WriteString)(EWRecord);           // put it to the logger
      }
    }
    EWMicroRecorderWriteWayPoint(d, wpLast, TEXT("Finish LatLon:    "));
    EWMicroRecorderWriteWayPoint(d, wpLast, TEXT("Land LatLon:      "));
    wpLast = NULL;
  }
  
  if (!fSimMode){
    (d->Com.WriteString)(TEXT("\x03"));         // finish sending user file

    if (!ExpectStringWait(d, TEXT("uploaded successfully"))) {
      nDeclErrorCode= 1;
      // error!
    }
    (d->Com.WriteString)(TEXT("!!\r\n"));         // go back to NMEA mode

    (d->Com.SetRxTimeout)(0);                       // clear timeout
    (d->Com.StartRxThread)();                       // restart RX thread
  }

  fDeclarationPending = FALSE;                    // clear decl pending flag

  return(nDeclErrorCode == 0);                    // return() TRUE on success

}



BOOL EWMicroRecorderDeclAddWayPoint(PDeviceDescriptor_t d, 
                                    WAYPOINT *wp){

  if (nDeclErrorCode != 0)                        // check for error
    return(FALSE);
  if (ewDecelTpIndex >= 12){                        // check for max 12 TP's
    return(FALSE);
  }

  if (ewDecelTpIndex==0) {
    wpLast = NULL;
    EWMicroRecorderWriteWayPoint(d, wp, TEXT("Take Off LatLong: "));
    EWMicroRecorderWriteWayPoint(d, wp, TEXT("Start LatLon:     "));
  } else {
    if (wpLast) {
      EWMicroRecorderWriteWayPoint(d, wpLast, TEXT("TP LatLon:        "));
    }
    wpLast = wp;
  }
        
  ewDecelTpIndex++;

  return(TRUE);

}


BOOL EWMicroRecorderIsLogger(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL EWMicroRecorderIsGPSSource(PDeviceDescriptor_t d){
  (void)d;
  return(TRUE);
}


BOOL ewMicroRecorderInstall(PDeviceDescriptor_t d){
  _tcscpy(d->Name, TEXT("EW MicroRecorder"));
  d->ParseNMEA = EWMicroRecorderParseNMEA;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->DeclBegin = EWMicroRecorderDeclBegin;
  d->DeclEnd = EWMicroRecorderDeclEnd;
  d->DeclAddWayPoint = EWMicroRecorderDeclAddWayPoint;
  d->IsLogger = EWMicroRecorderIsLogger;
  d->IsGPSSource = EWMicroRecorderIsGPSSource;

  return(TRUE);

}


BOOL ewMicroRecorderRegister(void){
  return(devRegister(
    TEXT("EW MicroRecorder"), 
    1l << dfGPS
      | 1l << dfLogger,
    ewMicroRecorderInstall
  ));
}



