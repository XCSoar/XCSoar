#if !defined(AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_AIRSPACE_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

typedef struct _AIRSPACE_ACK
{
  bool AcknowledgedToday;
  double AcknowledgementTime;
} AIRSPACE_ACK;

typedef struct _AIRSPACE_ALT
{
	double Altitude;
	double FL;
} AIRSPACE_ALT;

typedef struct _AIRSPACE_AREA
{
	TCHAR Name[NAME_SIZE + 1];
        int Type;
	AIRSPACE_ALT Base;
	AIRSPACE_ALT Top;
	unsigned FirstPoint;
	unsigned NumPoints;
	int Visible;
	double MinLatitude;
	double MaxLatitude;
	double MinLongitude;
	double MaxLongitude;
        rectObj bounds;
        AIRSPACE_ACK Ack;
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
	int Visible;
        AIRSPACE_ACK Ack;
        rectObj bounds;
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

#endif
