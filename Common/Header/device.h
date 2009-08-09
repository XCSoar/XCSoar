
#ifndef	DEVICE_H
#define	DEVICE_H

#include <windows.h>
#include "Sizes.h"
#include "Port.h"
#include "MapWindow.h"

#define DEVNAMESIZE  32
#define	NUMDEV		 2
#define	NUMREGDEV	 20

#define	devA()	    (&DeviceList[0])
#define	devB()	    (&DeviceList[1])
#define devAll()    (NULL)

typedef	enum {dfGPS, dfLogger, dfSpeed,	dfVario, dfBaroAlt,	dfWind, dfVoice, dfNmeaOut, dfRadio} DeviceFlags_t;

typedef struct Declaration {
  TCHAR PilotName[64];
  TCHAR AircraftType[32];
  TCHAR AircraftRego[32];
  int num_waypoints;
  const WAYPOINT *waypoint[MAXTASKPOINTS];
} Declaration_t;

typedef	struct DeviceDescriptor_t{
  int	Port;
  FILE  *fhLogFile;
  ComPort *Com;
  TCHAR	Name[DEVNAMESIZE+1];
  BOOL (*ParseNMEA)(DeviceDescriptor_t *d, TCHAR *String, NMEA_INFO *GPS_INFO);
  BOOL (*PutMacCready)(DeviceDescriptor_t	*d,	double McReady);
  BOOL (*PutBugs)(DeviceDescriptor_t *d, double	Bugs);
  BOOL (*PutBallast)(DeviceDescriptor_t	*d,	double Ballast);
  BOOL (*PutVolume)(DeviceDescriptor_t	*d,	int Volume);
  BOOL (*PutFreqActive)(DeviceDescriptor_t	*d,	double Freq);
  BOOL (*PutFreqStandby)(DeviceDescriptor_t	*d,	double Standby);
  BOOL (*Open)(DeviceDescriptor_t	*d,	int	Port);
  BOOL (*Close)(DeviceDescriptor_t *d);
  BOOL (*LinkTimeout)(DeviceDescriptor_t *d);
  BOOL (*Declare)(DeviceDescriptor_t *d, Declaration_t *decl);
  BOOL (*IsLogger)(DeviceDescriptor_t	*d);
  BOOL (*IsGPSSource)(DeviceDescriptor_t *d);
  BOOL (*IsBaroSource)(DeviceDescriptor_t *d);
  BOOL (*IsRadio)(DeviceDescriptor_t *d);
  BOOL (*PutQNH)(DeviceDescriptor_t *d, double NewQNH);
  BOOL (*OnSysTicker)(DeviceDescriptor_t *d);
  BOOL (*PutVoice)(DeviceDescriptor_t *d, TCHAR *Sentence);
  BOOL (*IsCondor)(DeviceDescriptor_t	*d);
  DeviceDescriptor_t *pDevPipeTo;
}DeviceDescriptor_t;

typedef	DeviceDescriptor_t *PDeviceDescriptor_t;

#define Port1WriteNMEA(s)	devWriteNMEAString(devA(), s)
#define Port2WriteNMEA(s)	devWriteNMEAString(devB(), s)

void devWriteNMEAString(PDeviceDescriptor_t d, const TCHAR *Text);
void VarioWriteNMEA(const TCHAR *Text);
void VarioWriteSettings(void);
PDeviceDescriptor_t devVarioFindVega(void);

typedef	struct{
  const TCHAR	         *Name;
  unsigned int		 Flags;
  BOOL   (*Installer)(PDeviceDescriptor_t d);
} DeviceRegister_t;



extern DeviceDescriptor_t	DeviceList[NUMDEV];
extern DeviceRegister_t   DeviceRegister[NUMREGDEV];
extern int DeviceRegisterCount;
extern DeviceDescriptor_t *pDevPrimaryBaroSource;
extern DeviceDescriptor_t *pDevSecondaryBaroSource;

BOOL devRegister(const TCHAR *Name,	int	Flags, BOOL (*Installer)(PDeviceDescriptor_t d));
BOOL devRegisterGetName(int Index, TCHAR *Name);

BOOL devInit(LPTSTR CommandLine);
BOOL devCloseAll(void);
PDeviceDescriptor_t devGetDeviceOnPort(int Port);
BOOL ExpectString(PDeviceDescriptor_t d, const TCHAR *token);
BOOL devHasBaroSource(void);

BOOL devParseNMEA(int portNum, TCHAR *String,	NMEA_INFO	*GPS_INFO);
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
BOOL devOpenLog(PDeviceDescriptor_t d, TCHAR *FileName);
BOOL devCloseLog(PDeviceDescriptor_t d);

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH);
BOOL devOnSysTicker(DeviceDescriptor_t *d);

BOOL devGetBaroAltitude(double *Value);

BOOL devPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence);


#endif
