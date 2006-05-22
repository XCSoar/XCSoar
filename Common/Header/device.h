
#ifndef	DEVICE_H
#define	DEVICE_H
 
#include <windows.h>
#include "sizes.h"
#include "MapWindow.h"

#define DEVNAMESIZE  32
#define	NUMDEV		 2
#define	NUMREGDEV	 10

#define	devA()	    (&DeviceList[0])
#define	devB()	    (&DeviceList[1])
#define devAll()    (NULL)

typedef	enum {dfGPS, dfLogger, dfSpeed,	dfVario, dfBaroAlt,	dfWind, dfVoice} DeviceFlags_t;

typedef struct{
  void (*WriteString)(TCHAR *Text);
  void (*WriteNMEAString)(TCHAR *Text);
  BOOL (*StopRxThread)(void);
  BOOL (*StartRxThread)(void);
  int  (*GetChar)(void);
  int  (*SetRxTimeout)(int Timeout);
  unsigned long (*SetBaudrate)(unsigned long BaudRate);
  int  (*Read)(void *Buffer, size_t Size);
}ComPortDriver_t;

typedef	struct DeviceDescriptor_t{
	int	Port;	 
  FILE  *fhLogFile;
  ComPortDriver_t Com;
	TCHAR	Name[DEVNAMESIZE+1];
	BOOL (*ParseNMEA)(DeviceDescriptor_t *d, TCHAR *String,	NMEA_INFO	*GPS_INFO);
	BOOL (*PutMacCready)(DeviceDescriptor_t	*d,	double McReady);
	BOOL (*PutBugs)(DeviceDescriptor_t *d, double	Bugs);
	BOOL (*PutBallast)(DeviceDescriptor_t	*d,	double Ballast);
	BOOL (*Open)(DeviceDescriptor_t	*d,	int	Port);
	BOOL (*Close)(DeviceDescriptor_t *d);
	BOOL (*Init)(DeviceDescriptor_t	*d);
	BOOL (*LinkTimeout)(DeviceDescriptor_t *d);
	BOOL (*DeclBegin)(DeviceDescriptor_t *d, TCHAR *PilotsName,	TCHAR	*Class,	TCHAR	*ID);
	BOOL (*DeclEnd)(DeviceDescriptor_t *d);
	BOOL (*DeclAddWayPoint)(DeviceDescriptor_t *d, WAYPOINT	*wp);
	BOOL (*IsLogger)(DeviceDescriptor_t	*d);
	BOOL (*IsGPSSource)(DeviceDescriptor_t *d);
	BOOL (*IsBaroSource)(DeviceDescriptor_t *d);
	BOOL (*PutQNH)(DeviceDescriptor_t *d, double NewQNH);
	BOOL (*OnSysTicker)(DeviceDescriptor_t *d);
  BOOL (*PutVoice)(DeviceDescriptor_t *d, TCHAR *Sentence);
  int PortNumber;
}DeviceDescriptor_t;

typedef	DeviceDescriptor_t *PDeviceDescriptor_t;

typedef	struct{
	TCHAR	 *Name;
	int		 Flags;
  BOOL   (*Installer)(PDeviceDescriptor_t d);
}DeviceRegister_t;



extern DeviceDescriptor_t	DeviceList[NUMDEV];
extern DeviceRegister_t   DeviceRegister[NUMREGDEV];
extern int DeviceRegisterCount;
extern DeviceDescriptor_t *pDevPrimaryBaroSource;
extern DeviceDescriptor_t *pDevSecondaryBaroSource;

BOOL devRegister(TCHAR *Name,	int	Flags, BOOL (*Installer)(PDeviceDescriptor_t d));
BOOL devRegisterGetName(int Index, TCHAR *Name);

BOOL devInit(LPTSTR CommandLine);
BOOL devCloseAll(void);
PDeviceDescriptor_t devGetDeviceOnPort(int Port);
BOOL ExpectString(PDeviceDescriptor_t d, TCHAR *token);

BOOL devParseNMEA(int portNum, TCHAR *String,	NMEA_INFO	*GPS_INFO);
BOOL devPutMacCready(PDeviceDescriptor_t d,	double MacCready);
BOOL devPutBugs(PDeviceDescriptor_t	d, double	Bugs);
BOOL devPutBallast(PDeviceDescriptor_t d,	double Ballast);
BOOL devOpen(PDeviceDescriptor_t d,	int	Port);
BOOL devClose(PDeviceDescriptor_t	d);
BOOL devInit(PDeviceDescriptor_t d);
BOOL devLinkTimeout(PDeviceDescriptor_t	d);
BOOL devDeclBegin(PDeviceDescriptor_t	d, TCHAR *PilotsName,	TCHAR	*Class,	TCHAR	*ID);
BOOL devDeclEnd(PDeviceDescriptor_t	d);
BOOL devDeclAddWayPoint(PDeviceDescriptor_t	d, WAYPOINT	*wp);
BOOL devIsLogger(PDeviceDescriptor_t d);
BOOL devIsGPSSource(PDeviceDescriptor_t	d);
BOOL devIsBaroSource(PDeviceDescriptor_t d);


BOOL devOpenLog(PDeviceDescriptor_t d, TCHAR *FileName);
BOOL devCloseLog(PDeviceDescriptor_t d);

BOOL devPutQNH(DeviceDescriptor_t *d, double NewQNH);
BOOL devOnSysTicker(DeviceDescriptor_t *d);

BOOL devGetBaroAltitude(double *Value);

BOOL devPutVoice(PDeviceDescriptor_t d, TCHAR *Sentence);


#endif
