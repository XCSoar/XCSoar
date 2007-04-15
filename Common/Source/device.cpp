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

#include "options.h"

#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "device.h"

#define debugIGNORERESPONCE 0

DeviceRegister_t   DeviceRegister[NUMREGDEV];
DeviceDescriptor_t DeviceList[NUMDEV];

DeviceDescriptor_t *pDevPrimaryBaroSource=NULL;
DeviceDescriptor_t *pDevSecondaryBaroSource=NULL;

int DeviceRegisterCount = 0;


BOOL devGetBaroAltitude(double *Value){
  // hack, just return GPS_INFO->BaroAltitude
  if (Value == NULL)
    return(FALSE);
  if (GPS_INFO.BaroAltitudeAvailable)
    *Value = GPS_INFO.BaroAltitude;
  return(TRUE);

  // ToDo
  // more than one baro source may be available
  // eg Altair (w. Logger) and intelligent vario
  // - which source should be used?
  // - whats happen if primary source fails
  // - plausibility check? against second baro? or GPS alt?
  // - whats happen if the diference is too big?

}

BOOL ExpectString(PDeviceDescriptor_t d, TCHAR *token){

  int i=0, ch;

  while ((ch = (d->Com.GetChar)()) != EOF){

    if (token[i] == ch)
      i++;
    else
      i=0;

    if ((unsigned)i == _tcslen(token))
      return(TRUE);

  }

  #if debugIGNORERESPONCE > 0
  return(TRUE);
  #endif
  return(FALSE);

}


BOOL devRegister(TCHAR *Name, int Flags, BOOL (*Installer)(PDeviceDescriptor_t d)){
  if (DeviceRegisterCount >= NUMREGDEV)
    return(FALSE);
  DeviceRegister[DeviceRegisterCount].Name = Name;
  DeviceRegister[DeviceRegisterCount].Flags = Flags;
  DeviceRegister[DeviceRegisterCount].Installer = Installer;
  DeviceRegisterCount++;
  return(TRUE);
}

BOOL devRegisterGetName(int Index, TCHAR *Name){
  Name[0] = '\0';
  if (Index < 0 || Index >= DeviceRegisterCount)
    return (FALSE);
  _tcscpy(Name, DeviceRegister[Index].Name);
  return(TRUE);
}


static int devIsFalseReturn(PDeviceDescriptor_t d){
  (void)d;
  return(FALSE);

}


BOOL devInit(LPTSTR CommandLine){
  int i;
  TCHAR DeviceName[DEVNAMESIZE];

  for (i=0; i<NUMDEV; i++){
    DeviceList[i].Port = -1;
    DeviceList[i].fhLogFile = NULL;
    DeviceList[i].Name[0] = '\0';
    DeviceList[i].ParseNMEA = NULL;
    DeviceList[i].PutMacCready = NULL;
    DeviceList[i].PutBugs = NULL;
    DeviceList[i].PutBallast = NULL;
    DeviceList[i].Open = NULL;
    DeviceList[i].Close = NULL;
    DeviceList[i].Init = NULL;
    DeviceList[i].LinkTimeout = NULL;
    DeviceList[i].DeclBegin = NULL;
    DeviceList[i].DeclEnd = NULL;
    DeviceList[i].DeclAddWayPoint = NULL;
    DeviceList[i].IsLogger = devIsFalseReturn;
    DeviceList[i].IsGPSSource = devIsFalseReturn;
    DeviceList[i].IsBaroSource = devIsFalseReturn;

    DeviceList[i].PutVoice = (int (*)(struct DeviceDescriptor_t *,TCHAR *))devIsFalseReturn;
    DeviceList[i].PortNumber = i;
    DeviceList[i].PutQNH = NULL;
    DeviceList[i].OnSysTicker = NULL;
  }

  pDevPrimaryBaroSource = NULL;
  pDevSecondaryBaroSource=NULL;

  ReadDeviceSettings(0, DeviceName);

  for (i=0; i<DeviceRegisterCount; i++){
    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){

      DeviceRegister[i].Installer(devA());

      // remember: Port1 is the port used by device A, port1 may be Com3 or Com1 etc
      devA()->Com.WriteString = Port1WriteString;
      devA()->Com.WriteNMEAString = Port1WriteNMEA;
      devA()->Com.StopRxThread = Port1StopRxThread;
      devA()->Com.StartRxThread = Port1StartRxThread;
      devA()->Com.GetChar = Port1GetChar;
      devA()->Com.SetRxTimeout = Port1SetRxTimeout;
      devA()->Com.SetBaudrate = Port1SetBaudrate;
      devA()->Com.Read = Port1Read;

      devInit(devA());
      devOpen(devA(), 0);

      if (devA()->IsBaroSource(devA())){
        if (pDevPrimaryBaroSource == NULL){
          pDevPrimaryBaroSource = devA();
        } else
        if (pDevSecondaryBaroSource == NULL){
          pDevSecondaryBaroSource = devA();
        }
      }

      break;
    }
  }


  ReadDeviceSettings(1, DeviceName);

  for (i=0; i<DeviceRegisterCount; i++){
    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){
      DeviceRegister[i].Installer(devB());


      devB()->Com.WriteString = Port2WriteString;
      devB()->Com.WriteNMEAString = Port2WriteNMEA;
      devB()->Com.StopRxThread = Port2StopRxThread;
      /* following methodes are not jet suppoerted
      devB()->Com.StartRxThread = Port2StartRxThread;
      devB()->Com.GetChar = Port2GetChar;
      devB()->Com.SetRxTimeout = Port2SetRxTimeout;
      devB()->Com.SetBaudrate = Port2SetBaudrate;
      devB()->Com.Read = Port2Read;
      */

      devInit(devB());
      devOpen(devB(), 1);

      if (devB()->IsBaroSource(devB())){
        if (pDevPrimaryBaroSource == NULL){
          pDevPrimaryBaroSource = devB();
        } else
        if (pDevSecondaryBaroSource == NULL){
          pDevSecondaryBaroSource = devB();
        }
      }

      break;
    }
  }

  CommandLine = LOGGDEVCOMMANDLINE;

  if (CommandLine != NULL){
    TCHAR *pC, *pCe;
    TCHAR wcLogFileName[MAX_PATH];
    TCHAR sTmp[128];

    pC = _tcsstr(CommandLine, TEXT("-logA="));
    if (pC != NULL){
      pC += strlen("-logA=");
      if (*pC == '"'){
        pC++;
        pCe = pC;
        while (*pCe != '"' && *pCe != '\0') pCe++;
      } else{
        pCe = pC;
        while (*pCe != ' ' && *pCe != '\0') pCe++;
      }
      if (pCe != NULL && pCe-1 > pC){

        _tcsncpy(wcLogFileName, pC, pCe-pC);
        wcLogFileName[pCe-pC] = '\0';

        if (devOpenLog(devA(), wcLogFileName)){
          _stprintf(sTmp, TEXT("Device A Loggs to\r\n%s"), wcLogFileName),
          MessageBox (hWndMainWindow, sTmp,
                TEXT("Information"), MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp, TEXT("Unable to Open Log\r\non Device A\r\n%s"), wcLogFileName),
          MessageBox (hWndMainWindow, sTmp,
                TEXT("Error"), MB_OK|MB_ICONWARNING);
        }

      }

    }

    pC = _tcsstr(CommandLine, TEXT("-logB="));
    if (pC != NULL){
      pC += strlen("-logA=");
      if (*pC == '"'){
        pC++;
        pCe = pC;
        while (*pCe != '"' && *pCe != '\0') pCe++;
      } else{
        pCe = pC;
        while (*pCe != ' ' && *pCe != '\0') pCe++;
      }
      if (pCe != NULL && pCe > pC){

        _tcsncpy(wcLogFileName, pC, pCe-pC);
        wcLogFileName[pCe-pC] = '\0';

        if (devOpenLog(devB(), wcLogFileName)){
          _stprintf(sTmp, TEXT("Device B Loggs to\r\n%s"), wcLogFileName),
          MessageBox (hWndMainWindow, sTmp,
                TEXT("Information"), MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp, TEXT("Unable to Open Log\r\non Device B\r\n%s"), wcLogFileName),
          MessageBox (hWndMainWindow, sTmp,
                TEXT("Error"), MB_OK|MB_ICONWARNING);
        }

      }

    }

  }

  return(TRUE);
}


BOOL devCloseAll(void){
  int i;

  for (i=0; i<NUMDEV; i++){
    devClose(&DeviceList[i]);
    devCloseLog(&DeviceList[i]);
  }
  return(TRUE);
}


PDeviceDescriptor_t devGetDeviceOnPort(int Port){

  int i;

  for (i=0; i<NUMDEV; i++){
    if (DeviceList[i].Port == Port)
      return(&DeviceList[i]);
  }
  return(NULL);
}



BOOL devParseNMEA(int portNum, TCHAR *String, NMEA_INFO *GPS_INFO){
  PDeviceDescriptor_t d;
  d = devGetDeviceOnPort(portNum);

  if ((d != NULL) &&
      (d->fhLogFile != NULL) &&
      (String != NULL) && (_tcslen(String) > 0)) {
    char  sTmp[500];  // temp multibyte buffer
    TCHAR *pWC = String;
    char  *pC  = sTmp;
    static DWORD lastFlush = 0;

    sprintf(pC, "%9d <", GetTickCount());
    pC = sTmp + strlen(sTmp);

    while (*pWC){
      if (*pWC != '\r'){
        *pC = (char)*pWC;
        pC++;
      }
      pWC++;
    }
    *pC++ = '>';
    *pC++ = '\r';
    *pC++ = '\n';
    *pC++ = '\0';

    fputs(sTmp, d->fhLogFile);

  }


  if (d != NULL && d->ParseNMEA != NULL)
    if ((d->ParseNMEA)(d, String, GPS_INFO))
      return(TRUE);

  if(String[0]=='$')  // Additional "if" to find GPS strings
    {

//      bool dodisplay = false;

      if(NMEAParser::ParseNMEAString(portNum, String, GPS_INFO))
        {
          GPSCONNECT  = TRUE;
	  return(TRUE);
        }
    }
  return(FALSE);
}


BOOL devPutMacCready(PDeviceDescriptor_t d, double MacCready){
  if (d != NULL && d->PutMacCready != NULL)
    return ((d->PutMacCready)(d, MacCready));
  else
    return(TRUE);
}

BOOL devPutBugs(PDeviceDescriptor_t d, double Bugs){
  if (d != NULL && d->PutBugs != NULL)
    return ((d->PutBugs)(d, Bugs));
  else
    return(TRUE);
}

BOOL devPutBallast(PDeviceDescriptor_t d, double Ballast){
  if (d != NULL && d->PutBallast != NULL)
    return ((d->PutBallast)(d, Ballast));
  else
    return(TRUE);
}

BOOL devOpen(PDeviceDescriptor_t d, int Port){
  if (d != NULL && d->Open != NULL)
    return ((d->Open)(d, Port));
  else
    return(TRUE);
}

BOOL devClose(PDeviceDescriptor_t d){
  if (d != NULL && d->Close != NULL)
    return ((d->Close)(d));
  else
    return(TRUE);
}

BOOL devInit(PDeviceDescriptor_t d){
  if (d != NULL && d->Init != NULL)
    return ((d->Init)(d));
  else
    return(TRUE);
}

BOOL devLinkTimeout(PDeviceDescriptor_t d){
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->LinkTimeout != NULL)
        (d->LinkTimeout)(d);
    }
    return (TRUE);
  } else {
    if (d->LinkTimeout != NULL)
      return ((d->LinkTimeout)(d));
  }
  return(FALSE);
}


BOOL devPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence){

  if (d == NULL){

    for (int i=0; i<NUMDEV; i++){

      d = &DeviceList[i];

      if (d->PutVoice != NULL)

        (d->PutVoice)(d, Sentence);

    }

    return (TRUE);

  } else {

    if (d->PutVoice != NULL)

      return ((d->PutVoice)(d, Sentence));

  }

  return(FALSE);

}


BOOL devDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){
  if (d != NULL && d->DeclBegin != NULL)
    return ((d->DeclBegin)(d, PilotsName, Class, ID));
  else
    return(FALSE);
}

BOOL devDeclEnd(PDeviceDescriptor_t d){
  if (d != NULL && d->DeclEnd != NULL)
    return ((d->DeclEnd)(d));
  else
    return(FALSE);
}

BOOL devDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){
  if (d != NULL && d->DeclAddWayPoint != NULL)
    return ((d->DeclAddWayPoint)(d, wp));
  else
    return(FALSE);
}

BOOL devIsLogger(PDeviceDescriptor_t d){
  if (d != NULL && d->IsLogger != NULL)
    return ((d->IsLogger)(d));
  else
    return(FALSE);
}

BOOL devIsGPSSource(PDeviceDescriptor_t d){
  if (d != NULL && d->IsGPSSource != NULL)
    return ((d->IsGPSSource)(d));
  else
    return(FALSE);
}

BOOL devIsBaroSource(PDeviceDescriptor_t d){
  if (d != NULL && d->IsBaroSource != NULL)
    return ((d->IsBaroSource)(d));
  else
    return(FALSE);
}

BOOL devOpenLog(PDeviceDescriptor_t d, TCHAR *FileName){
  if (d != NULL){
    d->fhLogFile = _tfopen(FileName, TEXT("a+b"));
    return(d->fhLogFile != NULL);
  } else
    return(FALSE);
}

BOOL devCloseLog(PDeviceDescriptor_t d){
  if (d != NULL && d->fhLogFile != NULL){
    fclose(d->fhLogFile);
    return(TRUE);
  } else
    return(FALSE);
}

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH){
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->PutQNH != NULL)
        (d->PutQNH)(d, NewQNH);
    }
    return(TRUE);
  } else {
    if (d->PutQNH != NULL)
      return ((d->PutQNH)(d, NewQNH));
  }
  return(FALSE);
}

BOOL devOnSysTicker(DeviceDescriptor_t *d){
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->OnSysTicker != NULL)
        (d->OnSysTicker)(d);
    }
    return(TRUE);
  } else {
    if (d->OnSysTicker != NULL)
      return ((d->OnSysTicker)(d));
  }
  return(FALSE);
}

