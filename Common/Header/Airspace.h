#if !defined(AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StdAfx.h"
#include "Sizes.h"
#include "mapshape.h"

#define OTHER                           0
#define RESTRICT                        1
#define PROHIBITED                      2
#define DANGER                          3
#define CLASSA				4
#define CLASSB				5
#define CLASSC				6
#define CLASSD				7
#define	NOGLIDER			8
#define CTR                             9
#define WAVE				10
#define AATASK				11
#define CLASSE				12
#define CLASSF				13
#define AIRSPACECLASSCOUNT              14

#define ALLON 0
#define CLIP 1
#define AUTO 2
#define ALLBELOW 3
#define INSIDE 4
#define ALLOFF 5


typedef struct _AIRSPACE_ACK
{
  bool AcknowledgedToday;
  double AcknowledgementTime;
} AIRSPACE_ACK;

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

typedef struct _AIRSPACE_ALT
{
  double Altitude;
  double FL;
  double AGL;
  AirspaceAltBase_t Base;  
} AIRSPACE_ALT;

typedef struct _AIRSPACE_AREA
{
  TCHAR Name[NAME_SIZE + 1];
  int Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;
  unsigned FirstPoint;
  unsigned NumPoints;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
  double MinLatitude;
  double MaxLatitude;
  double MinLongitude;
  double MaxLongitude;
  rectObj bounds;
  AIRSPACE_ACK Ack;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
  bool FarVisible;
} AIRSPACE_AREA;

typedef struct _AIRSPACE_POINT
{
  double Latitude;
  double Longitude;
} AIRSPACE_POINT;

typedef struct _AIRSPACE_CIRCLE
{
  TCHAR Name[NAME_SIZE + 1];
  int Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;
  double Latitude;
  double Longitude;
  double Radius;
  POINT Screen;
  int ScreenR;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
  AIRSPACE_ACK Ack;
  rectObj bounds;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
  bool FarVisible;
} AIRSPACE_CIRCLE;


void ReadAirspace(void);
int FindAirspaceCircle(double Longditude,double Lattitude,
		       bool visibleonly=true);
int FindAirspaceArea(double Longditude,double Lattitude,
		     bool visibleonly=true);
BOOL CheckAirspaceAltitude(const double &Base, const double &Top);
void CloseAirspace(void);

void FindNearestAirspace(double longitude, 
			 double latitude,
			 double *nearestdistance, 
			 double *nearestbearing,
			 int *foundcircle, 
			 int *foundarea,
			 double *height=NULL);

void SortAirspace(void);
extern int AirspacePriority[AIRSPACECLASSCOUNT];

bool InsideAirspaceCircle(const double &longitude,
			  const double &latitude,
			  int i);

bool InsideAirspaceArea(const double &longitude,
			const double &latitude,
			int i);

#define AIRSPACE_SCANSIZE_X 16
#define AIRSPACE_SCANSIZE_H 16

void ScanAirspaceLine(double *lats, double *lons, double *heights, 
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X]);


void AirspaceQnhChangeNotify(double newQNH);

//*******************************************************************************
// experimental: new dialog based warning system


#define OUTSIDE_CHECK_INTERVAL 4

class AirspaceInfo_c{

public:

  int    TimeOut;             // in systicks
  int    InsideAckTimeOut;    // downgrade auto ACK timer
  int    Sequence;            // Sequence nummer is equal for real and predicted calculation
  int    hDistance;           // horizontal distance in m
  int    vDistance;           // vertical distance in m
  int    Bearing;             // in deg
  DWORD  PredictedEntryTime;  // in ms
  int    Acknowledge;         // 0=not Acked, 1=Acked til closer, 2=Acked til leave, 3= Acked whole day
  bool   Inside;              // true if inside
  bool   Predicted;           // true if predicted inside, menas close and entry expected
  bool   IsCircle;            // true if Airspace is a circle
  int    AirspaceIndex;       // index of airspace
  int    SortKey;             // SortKey
  int    LastListIndex;       // Last index in List, used to sort items with same sort criteria
  int    ID;                  // Unique ID
  int    WarnLevel;           // WarnLevel 0 far away, 1 prdicted entry, 2 predicted entry and close, 3 inside      

};

typedef enum {asaNull, asaItemAdded, asaItemChanged, asaClearAll, asaItemRemoved, asaWarnLevelIncreased, asaProcessEnd, asaProcessBegin} AirspaceWarningNotifyAction_t;
typedef void (*AirspaceWarningNotifier_t)(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace) ;

void AirspaceWarnListAddNotifier(AirspaceWarningNotifier_t Notifier);
void AirspaceWarnListRemoveNotifier(AirspaceWarningNotifier_t Notifier);
bool AirspaceWarnGetItem(int Index, AirspaceInfo_c &Item);
int AirspaceWarnGetItemCount(void);
int dlgAirspaceWarningInit(void);
int dlgAirspaceWarningDeInit(void);
void AirspaceWarnListClear(void);
void AirspaceWarnDoAck(int ID, int Ack);
int AirspaceWarnFindIndexByID(int ID);
void AirspaceWarnListInit(void);
void AirspaceWarnListDeInit(void);

// MapWindow interface ...
bool dlgAirspaceWarningShowDlg(bool Force);

double ProjectedDistance(double lon1, double lat1,
                         double lon2, double lat2,
                         double lon3, double lat3);

bool ValidAirspace(void);

double RangeAirspaceCircle(const double &longitude,
			   const double &latitude,
			   int i);

double RangeAirspaceArea(const double &longitude,
			 const double &latitude,
			 int i, double *bearing);

void ScreenClosestPoint(const POINT &p1, const POINT &p2, 
			const POINT &p3, POINT *p4, int offset = 0);


#endif
