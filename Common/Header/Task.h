#if !defined(AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_TASK_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

#define CIRCLE 0
#define SECTOR 1

typedef struct _TASK_POINT
{
  int Index;
	double InBound;
	double OutBound;
	double Bisector;
	double Leg;
	double SectorStartLat;
	double SectorStartLon;
	double SectorEndLat;
	double SectorEndLon;
	POINT	 Start;
	POINT	 End;		
	int		 AATType;
	double AATCircleRadius;
	double AATSectorRadius;
	double AATStartRadial;
	double AATFinishRadial;
	double AATStartLat;
	double AATStartLon;
	double AATFinishLat;
	double AATFinishLon;
	POINT	 AATStart;
	POINT	 AATFinish;	
}TASK_POINT;


#endif