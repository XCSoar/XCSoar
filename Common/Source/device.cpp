
#include <windows.h>
#include <tchar.h>


#include "externs.h"
#include "utils.h"
#include "parser.h"
#include "port.h"

#include "device.h"

DeviceDescriptor_t DeviceList[NUMDEV];

static DeviceRegister_t   DeviceRegister[NUMREGDEV];

static DeviceRegisterCount = 0;

BOOL devRegister(TCHAR *Name, int Flags){
  if (DeviceRegisterCount >= NUMREGDEV)
    return(TRUE);
  DeviceRegister[DeviceRegisterCount].Name = Name;
  DeviceRegister[DeviceRegisterCount].Flags = Flags;
  DeviceRegisterCount++;
  return(TRUE);
}

BOOL devInit(void){
  int i;

  for (i=0; i<NUMDEV; i++){
    DeviceList[i].Port = -1;
    DeviceList[i].Name[0] = '\0';
    DeviceList[i].ParseNMEA = NULL;
    DeviceList[i].PutMcReady = NULL;
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

  if (d->ParseNMEA != NULL)
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


BOOL devPutMcReady(PDeviceDescriptor_t d, double McReady){
  if (d->PutMcReady != NULL)
    return ((d->PutMcReady)(d, McReady));
  else
    return(TRUE);
}

BOOL devPutBugs(PDeviceDescriptor_t d, double Bugs){
  if (d->PutBugs != NULL)
    return ((d->PutBugs)(d, Bugs));
  else
    return(TRUE);
}

BOOL devPutBallast(PDeviceDescriptor_t d, double Ballast){
  if (d->PutBallast != NULL)
    return ((d->PutBallast)(d, Ballast));
  else
    return(TRUE);
}

BOOL devOpen(PDeviceDescriptor_t d, int Port){
  if (d->Open != NULL)
    return ((d->Open)(d, Port));
  else
    return(TRUE);
}

BOOL devClose(PDeviceDescriptor_t d){
  if (d->Close != NULL)
    return ((d->Close)(d));
  else
    return(TRUE);
}

BOOL devInit(PDeviceDescriptor_t d){
  if (d->Init != NULL)
    return ((d->Init)(d));
  else
    return(TRUE);
}

BOOL devLinkTimeout(PDeviceDescriptor_t d){
  if (d->LinkTimeout != NULL)
    return ((d->LinkTimeout)(d));
  else
    return(TRUE);
}

BOOL devDeclBegin(PDeviceDescriptor_t d, TCHAR *PilotsName, TCHAR *Class, TCHAR *ID){
  if (d->DeclBegin != NULL)
    return ((d->DeclBegin)(d, PilotsName, Class, ID));
  else
    return(FALSE);
}

BOOL devDeclEnd(PDeviceDescriptor_t d){
  if (d->DeclEnd != NULL)
    return ((d->DeclEnd)(d));
  else
    return(FALSE);
}

BOOL devDeclAddWayPoint(PDeviceDescriptor_t d, WAYPOINT *wp){
  if (d->DeclAddWayPoint != NULL)
    return ((d->DeclAddWayPoint)(d, wp));
  else
    return(FALSE);
}

BOOL devIsLogger(PDeviceDescriptor_t d){
  if (d->IsLogger != NULL)
    return ((d->IsLogger)(d));
  else
    return(FALSE);
}

BOOL devIsGPSSource(PDeviceDescriptor_t d){
  if (d->IsGPSSource != NULL)
    return ((d->IsGPSSource)(d));
  else
    return(FALSE);
}

