
#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devEW.h"


#define  USESHORTTPNAME   1       // hack, soulf be configurable

// Additional sentance for EW support

static BOOL fDeclarationPending = FALSE;
static unsigned long lLastBaudrate = 0;
static nDeclErrorCode = 0;
static int ewDecelTpIndex = 0;


BOOL EWParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){

  // no propriatary sentence

  return FALSE;

}


BOOL EWOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  return(TRUE);
}

void appendCheckSum(TCHAR *String){
  int i;
  unsigned char CalcCheckSum = 0;
  TCHAR sTmp[4];

  for(i=1; String[i] != '\0'; i++){
    CalcCheckSum = CalcCheckSum ^ String[i];
  }

  _stprintf(sTmp, TEXT("%02X\r\n"), CalcCheckSum);
	_tcscat(String, sTmp);

}

BOOL EWDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  int retries=10;
  TCHAR sTmp[32];

  nDeclErrorCode = 0;
  ewDecelTpIndex = 0;
  fDeclarationPending = TRUE;

  (d->Com.StopRxThread)();
  lLastBaudrate = (d->Com.SetBaudrate)(9600L);    // change to IO Mode baudrate

  (d->Com.SetRxTimeout)(500);                     // set RX timeout to 500[ms]

  while (--retries){

    (d->Com.WriteString)(TEXT("##\r\n"));         // send IO Mode command
    if (ExpectString(d, TEXT("IO Mode.\r")))
      break;

    ExpectString(d, TEXT("$$$"));                 // empty imput buffer

  }

  if (retries <= 0){                              // to many retries
    nDeclErrorCode = 1;
    return(FALSE);
  };

  _stprintf(sTmp, TEXT("#SUI%02d"), 0);           // send pilot name
  appendCheckSum(sTmp);
  (d->Com.WriteString)(sTmp);
  Sleep(50);
  (d->Com.WriteString)(PilotsName);
  (d->Com.WriteString)(TEXT("\r"));

  if (!ExpectString(d, TEXT("OK\r"))){
    nDeclErrorCode = 1;
    return(FALSE);
  };

  _stprintf(sTmp, TEXT("#SUI%02d"), 1);           // send type of aircraft
  appendCheckSum(sTmp);
  (d->Com.WriteString)(sTmp);
  Sleep(50);
  (d->Com.WriteString)(Class);
  (d->Com.WriteString)(TEXT("\r"));

  if (!ExpectString(d, TEXT("OK\r"))){
    nDeclErrorCode = 1;
    return(FALSE);
  };

  _stprintf(sTmp, TEXT("#SUI%02d"), 2);           // send aircraft ID
  appendCheckSum(sTmp);
  (d->Com.WriteString)(sTmp);
  Sleep(50);
  (d->Com.WriteString)(ID);
  (d->Com.WriteString)(TEXT("\r"));

  if (!ExpectString(d, TEXT("OK\r"))){
    nDeclErrorCode = 1;
    return(FALSE);
  };

  for (int i=0; i<6; i++){                        // clear all 6 TP's
    _stprintf(sTmp, TEXT("#CTP%02d"), i);
    appendCheckSum(sTmp);
    (d->Com.WriteString)(sTmp);
    if (!ExpectString(d, TEXT("OK\r"))){
      nDeclErrorCode = 1;
      return(FALSE);
    };
  }

  return(TRUE);

}


BOOL EWDeclEnd(PDeviceDescriptor_t d){

  (d->Com.WriteString)(TEXT("NMEA\r\n"));         // switch to NMEA mode

  (d->Com.SetBaudrate)(lLastBaudrate);            // restore baudrate

  (d->Com.SetRxTimeout)(0);                       // clear timeout
  (d->Com.StartRxThread)();                       // restart RX thread

  fDeclarationPending = FALSE;                    // clear decl pending flag

  return(nDeclErrorCode == 0);                    // return() TRUE on success

}


BOOL EWDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  TCHAR EWRecord[100];
  TCHAR IDString[12];
  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;
  short EoW_Flag, NoS_Flag, EW_Flags;


  if (nDeclErrorCode != 0)                        // check for error
    return(FALSE);

  if (ewDecelTpIndex > 6){                        // check for max 6 TP's
    return(FALSE);
  }

	_tcsncpy(IDString, wp->Name, 6);                // copy at least 6 chars

  while (_tcslen(IDString) < 6)                   // fill up with spaces
    _tcscat(IDString, TEXT(" "));

  #if USESHORTTPNAME > 0
    _tcscpy(&IDString[3], TEXT("   "));           // truncate to short name
  #endif

  DegLat = (int)wp->Lattitude;                    // preparte lat
  MinLat = wp->Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)wp->Longditude ;                  // prepare long
  MinLon = wp->Longditude  - DegLon;
  EoW = 'E';
  if(MinLon<0)
    {
      EoW = 'W';
      DegLon *= -1; MinLon *= -1;
    }
  MinLon *=60;
  MinLon *= 1000;

  //	Calc E/W and N/S flags

  //	Clear flags
  EoW_Flag = 0;                                   // prepare flags
  NoS_Flag = 0;
  EW_Flags = 0;


  if (EoW == 'W')
    {
      EoW_Flag = 0x08;
    }
  else
    {
      EoW_Flag = 0x04;
    }
  if (NoS == 'N')
    {
      NoS_Flag = 0x01;
    }
  else
    {
      NoS_Flag = 0x02;
    }
  //  Do the calculation
  EW_Flags = EoW_Flag | NoS_Flag;

                                                  // setup command string
  _stprintf(EWRecord,TEXT("#STP%02X%02X%02X%02X%02X%02X%02X%02X%02X%04X%02X%04X"),
                      ewDecelTpIndex,
                      IDString[0],
                      IDString[1],
                      IDString[2],
                      IDString[3],
                      IDString[4],
                      IDString[5],
                      EW_Flags,
                      DegLat, (int)MinLat/10,
                      DegLon, (int)MinLon/10);

  appendCheckSum(EWRecord);                       // complete package with CS and CRLF

  (d->Com.WriteString)(EWRecord);                 // put it to the logger

  if (!ExpectString(d, TEXT("OK\r"))){            // wait for response
    nDeclErrorCode = 1;
    return(FALSE);
  };

  ewDecelTpIndex = ewDecelTpIndex + 1;            // increase TP index

  return(TRUE);

}


BOOL EWIsLogger(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL EWIsGPSSource(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL EWLinkTimeout(PDeviceDescriptor_t d){

  if (!fDeclarationPending)
    Port1WriteString(TEXT("NMEA\r\n"));

  return(TRUE);
}


BOOL ewInstall(PDeviceDescriptor_t d){
  _tcscpy(d->Name, TEXT("EW Logger"));
  d->ParseNMEA = EWParseNMEA;
  d->PutMcReady = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = EWLinkTimeout;
  d->DeclBegin = EWDeclBegin;
  d->DeclEnd = EWDeclEnd;
  d->DeclAddWayPoint = EWDeclAddWayPoint;
  d->IsLogger = EWIsLogger;
  d->IsGPSSource = EWIsGPSSource;

  return(TRUE);

}


BOOL ewRegister(void){
  return(devRegister(
    TEXT("EW Logger"),
    1l << dfGPS
      | 1l << dfLogger,
    ewInstall
  ));
}

