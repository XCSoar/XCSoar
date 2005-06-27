
#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "devEW.h"


// Additional sentance for EW support

static int EW_count = 0;
static int OK_Flag = 1; // Flag added to signal good or bad EW logger reply to turnpoint info
static int IO_Flag = 1; // Flag added to signal sucessful entry of EW logger into I/O Mode


BOOL EWParseNMEA(PDeviceDescriptor_t d, TCHAR *String, NMEA_INFO *GPS_INFO){

  OK_Flag = 1; // Set flag to failed state
  IO_Flag = 1; // Set flag to failed state

  if(_tcscmp(String, TEXT("OK\r")) == 0){
    OK_Flag = 0;
    return(TRUE);
  }

  if(_tcscmp(String, TEXT("IO Mode.\r")) == 0){
    IO_Flag = 0;
    return(TRUE);
  }

  return FALSE;

}


BOOL EWOpen(PDeviceDescriptor_t d, int Port){

  d->Port = Port;

  //TCHAR  szTmp[32];

  // _stprintf(szTmp, TEXT("%cLOG %d\r\n"), CtrlC, 0);

  // Port1WriteString(szTmp);
  return(TRUE);
}


BOOL EWDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){

  EW_count = 0;

  return(TRUE);

}


BOOL EWDeclEnd(PDeviceDescriptor_t d){

  return(TRUE);

}


BOOL EWDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){

  char EWRecord[100];
  char EWRecord_CS[100];

  char IDString[100];
  int i;

  int DegLat, DegLon;
  double MinLat, MinLon;
  char NoS, EoW;

  short EoW_Flag, NoS_Flag, EW_Flags;


  for(i=0;i<(int)_tcslen(wp->Name);i++)
    {
      IDString[i] = (char)wp->Name[i];
    }
  IDString[i] = '\0';

  char IDString_trunc[10];
  int j;


  for(j=0;j<3;j++)
    {
      IDString_trunc[j] = (char)wp->Name[j];
    }
  IDString_trunc[j] = '\0';



  DegLat = (int)wp->Lattitude;
  MinLat = wp->Lattitude - DegLat;
  NoS = 'N';
  if(MinLat<0)
    {
      NoS = 'S';
      DegLat *= -1; MinLat *= -1;
    }
  MinLat *= 60;
  MinLat *= 1000;


  DegLon = (int)wp->Longditude ;
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
  EoW_Flag = 0;
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

  // Temporary buffer to calculate checksum

  sprintf(EWRecord,"#STP0%X%X%X%X202020%02X%02X%04X%02X%04X", EW_count, IDString_trunc[0], IDString_trunc[1],IDString_trunc[2], EW_Flags, DegLat, (int)MinLat/10, DegLon, (int)MinLon/10);

  int l,len, m;
  unsigned char CalcCheckSum = 0;

  for(m=1; m<32; m++)
    {
      CalcCheckSum = CalcCheckSum ^ EWRecord[m];
    }

  sprintf(EWRecord_CS,"#STP0%X%X%X%X202020%02X%02X%04X%02X%04X%02X\r\n", EW_count, IDString_trunc[0], IDString_trunc[1],IDString_trunc[2], EW_Flags, DegLat, (int)MinLat/10, DegLon, (int)MinLon/10, CalcCheckSum);
  EW_count = EW_count + 1;

  len = strlen(EWRecord_CS);

  for(l=0;l<(len);l++)
    Port1Write ((BYTE)EWRecord_CS[l]);

  return(TRUE);

}


BOOL EWIsLogger(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL EWIsGPSSource(PDeviceDescriptor_t d){
  return(TRUE);
}


BOOL EWLinkTimeout(PDeviceDescriptor_t d){

  Port1WriteString(TEXT("NMEA\r\n"));

  return(TRUE);
}


BOOL ewInstall(PDeviceDescriptor_t d){
  _tcscpy(d->Name, TEXT("CAI 302"));
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
  return(devRegister(TEXT("EW Logger"),
      1l << dfGPS
    | 1l << dfLogger
  ));
}

