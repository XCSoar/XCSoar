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

// 20070413:sgi add NmeaOut support, allow nmea chaining an double port platforms

#include "StdAfx.h"

#include "options.h"

#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "device.h"

// A note about locking.
//  The ComPort RX threads lock using FlightData critical section.
//  ComPort::StopRxThread and ComPort::Close both wait for these threads to
//  exit before returning.  Both these functions are called with the Comm
//  critical section locked.  Therefore, there is a locking dependency upon
//  Comm -> FlightData.
//
//  If someone locks FlightData and then Comm, there is a high possibility
//  of deadlock.  So, FlightData must never be locked after Comm.  Ever.
//  Thankfully WinCE "critical sections" are recursive locks.

#define debugIGNORERESPONCE 0

static  const TCHAR *COMMPort[] = {TEXT("COM1:"),TEXT("COM2:"),TEXT("COM3:"),TEXT("COM4:"),TEXT("COM5:"),TEXT("COM6:"),TEXT("COM7:"),TEXT("COM8:"),TEXT("COM9:"),TEXT("COM10:"),TEXT("COM0:")};
static  DWORD   dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};
static  DWORD PortIndex1 = 0;
static  DWORD SpeedIndex1 = 2;
static  DWORD PortIndex2 = 0;
static  DWORD SpeedIndex2 = 2;

#ifdef _SIM_
static BOOL fSimMode = TRUE;
#else
static BOOL fSimMode = FALSE;
#endif

DeviceRegister_t   DeviceRegister[NUMREGDEV];
DeviceDescriptor_t DeviceList[NUMDEV];

DeviceDescriptor_t *pDevPrimaryBaroSource=NULL;
DeviceDescriptor_t *pDevSecondaryBaroSource=NULL;

int DeviceRegisterCount = 0;

// This function is used to determine whether a generic
// baro source needs to be used if available
BOOL devHasBaroSource(void) {
  if (pDevPrimaryBaroSource || pDevSecondaryBaroSource) {
    return TRUE;
  } else {
    return FALSE;
  }
}


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

BOOL ExpectString(PDeviceDescriptor_t d, const TCHAR *token){

  int i=0, ch;

  if (!d->Com)
    return FALSE;

  while ((ch = d->Com->GetChar()) != EOF){

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


BOOL devRegister(const TCHAR *Name, int Flags,
                 BOOL (*Installer)(PDeviceDescriptor_t d)) {
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
  PDeviceDescriptor_t pDevNmeaOut = NULL;

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
    DeviceList[i].Declare = NULL;
    DeviceList[i].IsLogger = devIsFalseReturn;
    DeviceList[i].IsGPSSource = devIsFalseReturn;
    DeviceList[i].IsBaroSource = devIsFalseReturn;

    DeviceList[i].PutVoice = (int (*)(struct DeviceDescriptor_t *,TCHAR *))devIsFalseReturn;
    DeviceList[i].PortNumber = i;
    DeviceList[i].PutQNH = NULL;
    DeviceList[i].OnSysTicker = NULL;

    DeviceList[i].pDevPipeTo = NULL;
  }

  pDevPrimaryBaroSource = NULL;
  pDevSecondaryBaroSource=NULL;

  ReadDeviceSettings(0, DeviceName);
#ifdef GNAV
  PortIndex1 = 2; SpeedIndex1 = 5;
#else
  PortIndex1 = 0; SpeedIndex1 = 2;
#endif
  ReadPort1Settings(&PortIndex1,&SpeedIndex1);

  for (i=0; i<DeviceRegisterCount; i++){
    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){
      ComPort *Com = new ComPort();

      // remember: Port1 is the port used by device A, port1 may be Com3 or Com1 etc
      if (!Com->Initialize(COMMPort[PortIndex1], dwSpeed[SpeedIndex1]))
        break;

      Com->ProcessChar = ProcessChar1;

      DeviceRegister[i].Installer(devA());

      if ((pDevNmeaOut == NULL) && (DeviceRegister[i].Flags & (1l << dfNmeaOut))){
        pDevNmeaOut = devA();
      }

      devA()->Com = Com;

      devInit(devA());
      devOpen(devA(), 0);

      if (devIsBaroSource(devA())) {
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
#ifdef GNAV
  PortIndex2 = 0; SpeedIndex2 = 5;
#else
  PortIndex2 = 0; SpeedIndex2 = 2;
#endif
  ReadPort2Settings(&PortIndex2,&SpeedIndex2);

  for (i=0; i<DeviceRegisterCount; i++){
    if (PortIndex1 == PortIndex2)
      break;

    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){
      ComPort *Com = new ComPort();

      if (!Com->Initialize(COMMPort[PortIndex2], dwSpeed[SpeedIndex2]))
        break;

      Com->ProcessChar = ProcessChar2;

      DeviceRegister[i].Installer(devB());

      if ((pDevNmeaOut == NULL) &&
          (DeviceRegister[i].Flags & (1l << dfNmeaOut))){
        pDevNmeaOut = devB();
      }

      devB()->Com = Com;

      devInit(devB());
      devOpen(devB(), 1);

      if (devIsBaroSource(devB())) {
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
          _stprintf(sTmp, TEXT("Device A logs to\r\n%s"), wcLogFileName);
          MessageBox (hWndMainWindow, sTmp,
                      gettext(TEXT("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp,
                    TEXT("Unable to open log\r\non device A\r\n%s"), wcLogFileName);
          MessageBox (hWndMainWindow, sTmp,
                      gettext(TEXT("Error")),
                      MB_OK|MB_ICONWARNING);
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
          _stprintf(sTmp, TEXT("Device B logs to\r\n%s"), wcLogFileName);
          MessageBox (hWndMainWindow, sTmp,
                      gettext(TEXT("Information")),
                      MB_OK|MB_ICONINFORMATION);
        } else {
          _stprintf(sTmp, TEXT("Unable to open log\r\non device B\r\n%s"),
                    wcLogFileName);
          MessageBox (hWndMainWindow, sTmp,
                      gettext(TEXT("Error")),
                      MB_OK|MB_ICONWARNING);
        }

      }

    }

  }

  if (pDevNmeaOut != NULL){
    if (pDevNmeaOut == devA()){
      devB()->pDevPipeTo = devA();
    }
    if (pDevNmeaOut == devB()){
      devA()->pDevPipeTo = devB();
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


  if (d != NULL){

    if (d->pDevPipeTo && d->pDevPipeTo->Com) {
      // stream pipe, pass nmea to other device (NmeaOut)
      // TODO ToDo check TX buffer usage and skip it if buffer is full (outbaudrate < inbaudrate)
      d->pDevPipeTo->Com->WriteString(String);
    }

    if (d->ParseNMEA != NULL)
      if ((d->ParseNMEA)(d, String, GPS_INFO))
        return(TRUE);
  }

  if(String[0]=='$')  // Additional "if" to find GPS strings
    {
      if(NMEAParser::ParseNMEAString(portNum, String, GPS_INFO))
        {
          GPSCONNECT  = TRUE;
          return(TRUE);
        }
    }

  return(FALSE);

}


BOOL devPutMacCready(PDeviceDescriptor_t d, double MacCready)
{
  BOOL result = TRUE;

  if (fSimMode)
    return TRUE;
  LockComm();
  if (d != NULL && d->PutMacCready != NULL)
    result = d->PutMacCready(d, MacCready);
  UnlockComm();

  return result;
}

BOOL devPutBugs(PDeviceDescriptor_t d, double Bugs)
{
  BOOL result = TRUE;

  if (fSimMode)
    return TRUE;
  LockComm();
  if (d != NULL && d->PutBugs != NULL)
    result = d->PutBugs(d, Bugs);
  UnlockComm();

  return result;
}

BOOL devPutBallast(PDeviceDescriptor_t d, double Ballast)
{
  BOOL result = TRUE;

  if (fSimMode)
    return TRUE;
  LockComm();
  if (d != NULL && d->PutBallast != NULL)
    result = d->PutBallast(d, Ballast);
  UnlockComm();

  return result;
}

// Only called from devInit() above which
// is in turn called with LockComm
BOOL devOpen(PDeviceDescriptor_t d, int Port){
  BOOL res = TRUE;

  if (d != NULL && d->Open != NULL)
    res = d->Open(d, Port);

  if (res == TRUE)
    d->Port = Port;

  return res;
}

// Tear down methods should always succeed.
// Called from devInit() above under LockComm
// Also called when shutting down via devCloseAll()
BOOL devClose(PDeviceDescriptor_t d)
{
  if (d != NULL) {
    if (d->Close != NULL)
      d->Close(d);

    ComPort *Com = d->Com;
    d->Com = NULL;

    if (Com) {
      Com->Close();
      delete Com;
    }
  }

  return TRUE;
}

// Only called from devInit() above which
// is in turn called with LockComm
BOOL devInit(PDeviceDescriptor_t d){
  if (d != NULL && d->Init != NULL)
    return ((d->Init)(d));
  else
    return(TRUE);
}

BOOL devLinkTimeout(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  if (fSimMode)
    return TRUE;
  LockComm();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->LinkTimeout != NULL)
        (d->LinkTimeout)(d);
    }
    result = TRUE;
  } else {
    if (d->LinkTimeout != NULL)
      result = d->LinkTimeout(d);
  }
  UnlockComm();

  return FALSE;
}


BOOL devPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence)
{
  BOOL result = FALSE;

  LockComm();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->PutVoice != NULL)
        d->PutVoice(d, Sentence);
    }
    result = TRUE;
  } else {
    if (d->PutVoice != NULL)
      result = d->PutVoice(d, Sentence);
  }
  UnlockComm();

  return FALSE;
}

BOOL devDeclare(PDeviceDescriptor_t d, Declaration_t *decl)
{
  BOOL result = FALSE;

  if (fSimMode)
    return TRUE;
  LockComm();
  if (d != NULL && d->Declare != NULL)
    result = d->Declare(d, decl);
  UnlockComm();

  return result;
}

BOOL devIsLogger(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  LockComm();
  if ((d != NULL) && (d->IsLogger != NULL))
    result = d->IsLogger(d);
  UnlockComm();

  return result;
}

BOOL devIsGPSSource(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  LockComm();
  if ((d != NULL) && (d->IsGPSSource != NULL))
    result = d->IsGPSSource(d);
  UnlockComm();

  return result;
}

BOOL devIsBaroSource(PDeviceDescriptor_t d)
{
  BOOL result = FALSE;

  LockComm();
  if ((d != NULL) && (d->IsBaroSource != NULL))
    result = d->IsBaroSource(d);
  UnlockComm();

  return result;
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

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH)
{
  BOOL result = FALSE;

  LockComm();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->PutQNH != NULL)
        d->PutQNH(d, NewQNH);
    }
    result = TRUE;
  } else {
    if (d->PutQNH != NULL)
      result = d->PutQNH(d, NewQNH);
  }
  UnlockComm();

  return FALSE;
}

BOOL devOnSysTicker(DeviceDescriptor_t *d)
{
  BOOL result = FALSE;

  LockComm();
  if (d == NULL){
    for (int i=0; i<NUMDEV; i++){
      d = &DeviceList[i];
      if (d->OnSysTicker != NULL)
        d->OnSysTicker(d);
    }
    result = TRUE;
  } else {
    if (d->OnSysTicker != NULL)
      result = d->OnSysTicker(d);
  }
  UnlockComm();

  return result;
}

static void devFormatNMEAString(TCHAR *dst, size_t sz, const TCHAR *text)
{
  BYTE chk;
  int i, len = _tcslen(text);

  for (chk = i = 0; i < len; i++)
    chk ^= (BYTE)text[i];

  _sntprintf(dst, sz, TEXT("$%s*%02X\r\n"), text, chk);
}

void devWriteNMEAString(PDeviceDescriptor_t d, const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  LockComm();
  if (d->Com)
    d->Com->WriteString(tmp);
  UnlockComm();
}

void VarioWriteNMEA(const TCHAR *text)
{
  TCHAR tmp[512];

  devFormatNMEAString(tmp, 512, text);

  LockComm();
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, TEXT("Vega")) == 0)
      if (DeviceList[i].Com)
        DeviceList[i].Com->WriteString(tmp);
  UnlockComm();
}

void VarioWriteSettings(void)
{
  if (GPS_INFO.VarioAvailable) {
    // JMW experimental
    TCHAR mcbuf[100];
    wsprintf(mcbuf, TEXT("PDVMC,%d,%d,%d,%d,%d"),
	     iround(MACCREADY*10),
	     iround(CALCULATED_INFO.VOpt*10),
	     CALCULATED_INFO.Circling,
	     iround(CALCULATED_INFO.TerrainAlt),
	     iround(QNH*10));
    VarioWriteNMEA(mcbuf);
  }
}

PDeviceDescriptor_t devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (_tcscmp(DeviceList[i].Name, TEXT("Vega")) == 0)
      return &DeviceList[i];
  return NULL;
}
