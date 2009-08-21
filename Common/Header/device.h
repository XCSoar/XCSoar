
#ifndef	DEVICE_H
#define	DEVICE_H

#include <windows.h>
#include "Sizes.h"
#include "Port.h"
#include "WayPoint.hpp"
#include "Parser.h"

#include <stdio.h>

#define DEVNAMESIZE  32
#define	NUMDEV		 2
#define	NUMREGDEV	 20

#define	devA()	    (&DeviceList[0])
#define	devB()	    (&DeviceList[1])
#define devAll()    (NULL)

typedef	enum {dfGPS, dfLogger, dfSpeed,	dfVario, dfBaroAlt,	dfWind, dfVoice, dfNmeaOut, dfRadio, dfCondor } DeviceFlags_t;

#define drfGPS		(1l << dfGPS)
#define drfLogger	(1l << dfLogger)
#define drfSpeed	(1l << dfSpeed)
#define drfVario	(1l << dfVario)
#define drfBaroAlt	(1l << dfBaroAlt)
#define drfWind		(1l << dfWind)
#define drfVoice	(1l << dfVoice)
#define drfNmeaOut	(1l << dfNmeaOut)
#define drfRadio	(1l << dfRadio)
#define drfCondor	(1l << dfCondor)

typedef struct Declaration {
  TCHAR PilotName[64];
  TCHAR AircraftType[32];
  TCHAR AircraftRego[32];
  int num_waypoints;
  const WAYPOINT *waypoint[MAXTASKPOINTS];
} Declaration_t;

struct DeviceRegister;

typedef	struct DeviceDescriptor_t{
  int	Port;
  FILE  *fhLogFile;
  ComPort *Com;
  TCHAR	Name[DEVNAMESIZE+1];
  DeviceDescriptor_t *pDevPipeTo;
  struct DeviceRegister *Driver;
  bool ticker;
}DeviceDescriptor_t;

typedef	DeviceDescriptor_t *PDeviceDescriptor_t;

#define Port1WriteNMEA(s)	devWriteNMEAString(devA(), s)
#define Port2WriteNMEA(s)	devWriteNMEAString(devB(), s)

void devWriteNMEAString(PDeviceDescriptor_t d, const TCHAR *Text);
void VarioWriteNMEA(const TCHAR *Text);
PDeviceDescriptor_t devVarioFindVega(void);

typedef	struct DeviceRegister {
  const TCHAR	*Name;
  unsigned int	Flags;
  BOOL (*ParseNMEA)(DeviceDescriptor_t *d, const TCHAR *String,
                    NMEA_INFO *GPS_INFO);
  BOOL (*PutMacCready)(DeviceDescriptor_t *d, double McReady);
  BOOL (*PutBugs)(DeviceDescriptor_t *d, double	Bugs);
  BOOL (*PutBallast)(DeviceDescriptor_t	*d, double Ballast);
  BOOL (*PutQNH)(DeviceDescriptor_t *d, double NewQNH);
  BOOL (*PutVoice)(DeviceDescriptor_t *d, const TCHAR *Sentence);
  BOOL (*PutVolume)(DeviceDescriptor_t *d, int Volume);
  BOOL (*PutFreqActive)(DeviceDescriptor_t *d, double Freq);
  BOOL (*PutFreqStandby)(DeviceDescriptor_t *d, double Standby);
  BOOL (*Open)(DeviceDescriptor_t *d, int Port);
  BOOL (*Close)(DeviceDescriptor_t *d);
  BOOL (*LinkTimeout)(DeviceDescriptor_t *d);
  BOOL (*Declare)(DeviceDescriptor_t *d, Declaration_t *decl);
  BOOL (*IsLogger)(DeviceDescriptor_t *d);
  BOOL (*IsGPSSource)(DeviceDescriptor_t *d);
  BOOL (*IsBaroSource)(DeviceDescriptor_t *d);
  BOOL (*OnSysTicker)(DeviceDescriptor_t *d);
} DeviceRegister_t;



extern DeviceDescriptor_t	DeviceList[NUMDEV];
extern DeviceRegister_t   DeviceRegister[NUMREGDEV];
extern int DeviceRegisterCount;
extern DeviceDescriptor_t *pDevPrimaryBaroSource;
extern DeviceDescriptor_t *pDevSecondaryBaroSource;

BOOL devRegister(const DeviceRegister_t *devReg);
BOOL devRegisterGetName(int Index, TCHAR *Name);

BOOL devInit(LPCTSTR CommandLine);
BOOL ExpectString(PDeviceDescriptor_t d, const TCHAR *token);
BOOL devHasBaroSource(void);

BOOL devParseNMEA(PDeviceDescriptor_t d, const TCHAR *String,
                  NMEA_INFO *GPS_INFO);
BOOL devPutMacCready(PDeviceDescriptor_t d,	double MacCready);
BOOL devPutBugs(PDeviceDescriptor_t	d, double	Bugs);
BOOL devPutBallast(PDeviceDescriptor_t d,	double Ballast);
BOOL devPutVolume(PDeviceDescriptor_t	d, int Volume);
BOOL devPutFreqActive(PDeviceDescriptor_t d,	double Freq);
BOOL devPutFreqStandby(PDeviceDescriptor_t d,	double Freq);
BOOL devOpen(PDeviceDescriptor_t d,	int	Port);
BOOL devClose(PDeviceDescriptor_t	d);
BOOL devLinkTimeout(PDeviceDescriptor_t	d);
BOOL devDeclare(PDeviceDescriptor_t	d, Declaration_t *decl);
BOOL devIsLogger(PDeviceDescriptor_t d);
BOOL devIsGPSSource(PDeviceDescriptor_t	d);
BOOL devIsBaroSource(PDeviceDescriptor_t d);
BOOL devIsRadio(PDeviceDescriptor_t d);
BOOL devIsCondor(PDeviceDescriptor_t d);
BOOL devOpenLog(PDeviceDescriptor_t d, const TCHAR *FileName);
BOOL devCloseLog(PDeviceDescriptor_t d);

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH);
void devTick(void);

BOOL devGetBaroAltitude(double *Value);

BOOL devPutVoice(PDeviceDescriptor_t d, const TCHAR *Sentence);


BOOL devIsFalseReturn(PDeviceDescriptor_t d);
BOOL devIsTrueReturn(PDeviceDescriptor_t d);

void devStartup(LPTSTR lpCmdLine);
void devShutdown();

#endif
