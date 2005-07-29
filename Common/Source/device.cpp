
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
int DeviceRegisterCount = 0;


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

BOOL decRegisterGetName(int Index, TCHAR *Name){
  Name[0] = '\0';
  if (Index < 0 || Index >= DeviceRegisterCount)
    return (FALSE);
  _tcscpy(Name, DeviceRegister[Index].Name);
  return(TRUE);
}

BOOL devInit(LPTSTR CommandLine){
  int i;
  TCHAR DeviceName[DEVNAMESIZE];

  for (i=0; i<NUMDEV; i++){
    DeviceList[i].Port = -1;
    DeviceList[i].fhLogFile = NULL;
    DeviceList[i].Name[0] = '\0';
    DeviceList[i].ParseNMEA = NULL;
    DeviceList[i].PutMcCready = NULL;
    DeviceList[i].PutBugs = NULL;
    DeviceList[i].PutBallast = NULL;
    DeviceList[i].Open = NULL;
    DeviceList[i].Close = NULL;
    DeviceList[i].Init = NULL;
    DeviceList[i].LinkTimeout = NULL;
    DeviceList[i].DeclBegin = NULL;
    DeviceList[i].DeclEnd = NULL;
    DeviceList[i].DeclAddWayPoint = NULL;
    DeviceList[i].IsLogger = NULL;
    DeviceList[i].IsGPSSource = NULL;
  }

  ReadDeviceSettings(0, DeviceName);

  for (i=0; i<DeviceRegisterCount; i++){
    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){
      DeviceRegister[i].Installer(devA());

      devA()->Com.WriteString = Port1WriteString;
      devA()->Com.StopRxThread = Port1StopRxThread;
      devA()->Com.StartRxThread = Port1StartRxThread;
      devA()->Com.GetChar = Port1GetChar;
      devA()->Com.SetRxTimeout = Port1SetRxTimeout;
      devA()->Com.SetBaudrate = Port1SetBaudrate;
      devA()->Com.Read = Port1Read;

      devInit(devA());
      devOpen(devA(), 0);

      break;
    }
  }


  ReadDeviceSettings(1, DeviceName);

  for (i=0; i<DeviceRegisterCount; i++){
    if (_tcscmp(DeviceRegister[i].Name, DeviceName) == 0){
      DeviceRegister[i].Installer(devB());

/* todo port2 driver's
      devB()->Com.WriteString = Port2WriteString;
      devB()->Com.StopRxThread = Port2StopRxThread;
      devB()->Com.StartRxThread = Port2StartRxThread;
      devB()->Com.GetChar = Port2GetChar;
      devB()->Com.SetRxTimeout = Port2SetRxTimeout;
      devB()->Com.SetBaudrate = Port2SetBaudrate;
      devB()->Com.Read = Port2Read;
*/
      devInit(devB());
      devOpen(devB(), 1);

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



BOOL devParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){


  if (d != NULL && d->fhLogFile != NULL && String != NULL && _tcslen(String) > 0){
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

      bool dodisplay = false;

      if(ParseNMEAString(String, GPS_INFO))
        {
          GPSCONNECT  = TRUE;
          if(GPS_INFO->NAVWarning == FALSE)
            {
/* JMW: wait for main thread to do this,
so don't get multiple updates
              if(DoCalculations(&GPS_INFO,&CALCULATED_INFO))
                {
                  AssignValues();
                  dodisplay = true;
                }
*/
            }
        return(TRUE);
        }
    }
  return(FALSE);
}


BOOL devPutMcCready(PDeviceDescriptor_t d, double McCready){
  if (d != NULL && d->PutMcCready != NULL)
    return ((d->PutMcCready)(d, McCready));
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
  if (d != NULL && d->LinkTimeout != NULL)
    return ((d->LinkTimeout)(d));
  else
    return(TRUE);
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

BOOL devOpenLog(PDeviceDescriptor_t d, TCHAR *FileName){
  if (d != NULL){
    d->fhLogFile = _tfopen(FileName, TEXT("wb"));
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
