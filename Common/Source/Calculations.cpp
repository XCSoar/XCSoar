/*
XCSoar Glide Computer
Copyright (C) 2000 - 2004  M Roberts

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "stdafx.h"
#include "Calculations.h"
#include "parser.h"
#include "Utils.h"
#include "Externs.h"
#include "McReady.h"
#include "Airspace.h"

#include <windows.h>
#include <math.h>

#include <tchar.h>

static void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int	InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int	InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int	InAATStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int	InAATurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static TERRAIN_INFO TerrainInfo;
static HANDLE hTerrain;

BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastTime = 0;

	DistanceToNext(Basic, Calculated);
	AltitudeRequired(Basic, Calculated);

	TerrainHeight(Basic, Calculated);;
	TaskStatistics(Basic, Calculated);


	FinalGlideAlert(Basic, Calculated);


	if(Basic->Time <= LastTime)
	{
		LastTime = Basic->Time;
		return FALSE;
	}

	LastTime = Basic->Time;

	Turning(Basic, Calculated);
	Vario(Basic,Calculated);
	LD(Basic,Calculated);
	CruiseLD(Basic,Calculated);
	Average30s(Basic,Calculated);
	AverageThermal(Basic,Calculated);
	ThermalGain(Basic,Calculated);
	LastThermalStats(Basic, Calculated);
	DistanceToNext(Basic, Calculated);
	AltitudeRequired(Basic, Calculated);
	InSector(Basic, Calculated);
	InAATSector(Basic, Calculated);

	TerrainHeight(Basic, Calculated);
	TaskStatistics(Basic, Calculated);
	AATStats(Basic, Calculated);


	CalculateNextPosition(Basic, Calculated);

	AirspaceWarning(Basic, Calculated);


	return TRUE;
}

void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastTime = 0;
	static double LastAlt = 0;
	double Gain;

	if(Basic->Time > LastTime)
	{
			Gain = Basic->Altitude - LastAlt;
			Calculated->Vario = Gain / (Basic->Time - LastTime);
			LastAlt = Basic->Altitude;
			LastTime = Basic->Time;
	}
	else
	{
		LastTime = Basic->Time;
	}
}

void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastTime = 0;
	static double Altitude[30];
	int Elapsed, i;
	long temp;
	double Gain;


	if(Basic->Time > LastTime)
	{
		Elapsed = (int)(Basic->Time - LastTime);
		for(i=0;i<Elapsed;i++)
		{
			temp = (long)LastTime + i;
			temp %=30;

			Altitude[temp] = Basic->Altitude;
		}
		temp = (long)Basic->Time - 1;
		temp = temp%30;
		Gain = Altitude[temp];

		temp = (long)Basic->Time;
		temp = temp%30;
		Gain = Gain - Altitude[temp];

		LastTime = Basic->Time;
		Calculated->Average30s = Gain/30;
	}
	else
	{
		LastTime = Basic->Time;
	}


}

void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	double Gain;

	if(Basic->Time > Calculated->ClimbStartTime)
	{
			Gain = Basic->Altitude - Calculated->ClimbStartAlt;
			Calculated->AverageThermal  = Gain / (Basic->Time - Calculated->ClimbStartTime);
	}
}

void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	if(Basic->Time > Calculated->ClimbStartTime)
	{
			Calculated->ThermalGain = Basic->Altitude - Calculated->ClimbStartAlt;
	}
}

void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastLat = 0;
	static double LastLon = 0;
	static double LastTime = 0;
	static double LastAlt = 0;
	double DistanceFlown;
	double AltLost;


	if(Basic->Time - LastTime >20)
	{
		DistanceFlown = Distance(Basic->Lattitude, Basic->Longditude, LastLat, LastLon);
		AltLost = LastAlt - Basic->Altitude;
		if(AltLost > 0)
		{
			Calculated->LD = DistanceFlown / AltLost;
			if(Calculated->LD>999)
			{
				Calculated->LD = 999;
			}
		}
		else
			Calculated->LD = 999;

		LastLat = Basic->Lattitude;
		LastLon = Basic->Longditude;
		LastAlt = Basic->Altitude;
		LastTime = Basic->Time;
	}
}

void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastLat = 0;
	static double LastLon = 0;
	static double LastTime = 0;
	static double LastAlt = 0;
	double DistanceFlown;
	double AltLost;


	if(!Calculated->Circling)
	{

		DistanceFlown = Distance(Basic->Lattitude, Basic->Longditude, Calculated->CruiseStartLat, Calculated->CruiseStartLong);
		AltLost = Calculated->CruiseStartAlt - Basic->Altitude;
		if(AltLost > 0)
		{
			Calculated->CruiseLD = DistanceFlown / AltLost;
			if(Calculated->CruiseLD>999)
			{
				Calculated->CruiseLD = 999;
			}
		}
		else
			Calculated->CruiseLD = 999;
	}
}

#define CRUISE 0
#define WAITCLIMB 1
#define CLIMB 2
#define WAITCRUISE 3


double MinTurnRate = 4 ; //10;
double CruiseClimbSwitch = 15;
double ClimbCruiseSwitch = 15;

void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static double LastTrack = 0;
	static double StartTime  = 0;
	static double StartLong = 0;
	static double StartLat = 0;
	static double StartAlt = 0;
	static double LastTime = 0;
	static int MODE = CRUISE;
	double Rate;

	if(Basic->Time <= LastTime)
	return;

	if((LastTrack>270) && (Basic->TrackBearing <90))
	{
		Rate = Basic->TrackBearing + (360-LastTrack);
	}
	else if ((LastTrack<90) && (Basic->TrackBearing >270))
	{
		Rate = LastTrack + (360-Basic->TrackBearing );
	}
	else
	{
		Rate = (Basic->TrackBearing - LastTrack);
	}
	Rate = Rate / (Basic->Time - LastTime);

	if(Rate <0)
	{
		Rate *= -1;
	}
	LastTime = Basic->Time;
	LastTrack = Basic->TrackBearing;

	if(MODE == CRUISE)
	{
		if(Rate > MinTurnRate)
		{
			StartTime = Basic->Time;
			StartLong = Basic->Longditude;
			StartLat  = Basic->Lattitude;
			StartAlt  = Basic->Altitude;
			MODE = WAITCLIMB;
		}
	}
	else if(MODE == WAITCLIMB)
	{
		if(Rate > MinTurnRate)
		{
			if( (Basic->Time  - StartTime) > CruiseClimbSwitch)
			{
				Calculated->Circling = TRUE;
				MODE = CLIMB;
				Calculated->ClimbStartLat = StartLat;
				Calculated->ClimbStartLong = StartLong;
				Calculated->ClimbStartAlt = StartAlt;
				Calculated->ClimbStartTime = StartTime;
			}
		}
		else
		{
			MODE = CRUISE;
		}
	}
	else if(MODE == CLIMB)
	{
		if(Rate < MinTurnRate)
		{
			StartTime = Basic->Time;
			StartLong = Basic->Longditude;
			StartLat  = Basic->Lattitude;
			StartAlt  = Basic->Altitude;
			MODE = WAITCRUISE;
		}
	}
	else if(MODE == WAITCRUISE)
	{
		if(Rate < MinTurnRate)
		{
			if( (Basic->Time  - StartTime) > ClimbCruiseSwitch)
			{
				Calculated->Circling = FALSE;
				MODE = CRUISE;
				Calculated->CruiseStartLat = StartLat;
				Calculated->CruiseStartLong = StartLong;
				Calculated->CruiseStartAlt = StartAlt;
				Calculated->CruiseStartTime = StartTime;
			}
		}
		else
		{
			MODE = CLIMB;
		}
	}
}

static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
		static int LastCircling = FALSE;
		double ThermalGain;
		double ThermalTime;
		double ThermalDrift;
		double DriftAngle;

		if((Calculated->Circling == FALSE) && (LastCircling == TRUE))
		{
			ThermalGain = Calculated->CruiseStartAlt - Calculated->ClimbStartAlt;
			ThermalTime = Calculated->CruiseStartTime - Calculated->ClimbStartTime;

			ThermalDrift = Distance(Calculated->CruiseStartLat,  Calculated->CruiseStartLong, Calculated->ClimbStartLat,  Calculated->ClimbStartLong);
			DriftAngle = Bearing(Calculated->ClimbStartLat,  Calculated->ClimbStartLong,Calculated->CruiseStartLat, Calculated->CruiseStartLong);

			if(ThermalTime >0)
			{
				Calculated->LastThermalAverage = ThermalGain/ThermalTime;
				Calculated->LastThermalGain = ThermalGain;
				Calculated->LastThermalTime = ThermalTime;
				if(ThermalTime > 120)
				{
					Calculated->WindSpeed = ThermalDrift/ThermalTime;

					if(DriftAngle >=180)
						DriftAngle -= 180;
					else
						DriftAngle += 180;

					Calculated->WindBearing = DriftAngle;
				}
			}
		}
		LastCircling = Calculated->Circling;
}

void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	if(ActiveWayPoint >=0)
	{
		Basic->WaypointDistance = Distance(Basic->Lattitude, Basic->Longditude,
																				WayPointList[Task[ActiveWayPoint].Index].Lattitude,
																				WayPointList[Task[ActiveWayPoint].Index].Longditude);
		Basic->WaypointBearing = Bearing(Basic->Lattitude, Basic->Longditude,
																				WayPointList[Task[ActiveWayPoint].Index].Lattitude,
																				WayPointList[Task[ActiveWayPoint].Index].Longditude);
	}
	else
	{
		Basic->WaypointDistance = 0;
	}
}

void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	if(ActiveWayPoint >=0)
	{
		Calculated->NextAltitudeRequired = McReadyAltitude(MACREADY/LIFTMODIFY,Basic->WaypointDistance,Basic->WaypointBearing, Calculated->WindSpeed, Calculated->WindBearing);
		Calculated->NextAltitudeRequired = Calculated->NextAltitudeRequired * (1/BUGS);
		Calculated->NextAltitudeRequired = Calculated->NextAltitudeRequired + SAFTEYALTITUDE ;

		Calculated->NextAltitudeDifference = Basic->Altitude - (Calculated->NextAltitudeRequired + WayPointList[Task[ActiveWayPoint].Index].Altitude);
	}
	else
	{
		Calculated->NextAltitudeRequired = 0;
		Calculated->NextAltitudeDifference = 0;
	}
}

int InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	double AircraftBearing;

	if(FAISector !=  TRUE)
	{
		if(Basic->WaypointDistance < SectorRadius)
		{
			return TRUE;
		}
	}
		// else
	{
		AircraftBearing = Bearing(WayPointList[Task[ActiveWayPoint].Index].Lattitude,
																WayPointList[Task[ActiveWayPoint].Index].Longditude,
																Basic->Lattitude ,
																Basic->Longditude);

		AircraftBearing = AircraftBearing - Task[ActiveWayPoint].Bisector ;

		if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
		{
			if(Basic->WaypointDistance < 20000)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

int InAATTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	double AircraftBearing;

	if(Task[ActiveWayPoint].AATType ==  CIRCLE)
	{
		if(Basic->WaypointDistance < Task[ActiveWayPoint].AATCircleRadius)
		{
			return TRUE;
		}
	}
	else if(Basic->WaypointDistance < Task[ActiveWayPoint].AATSectorRadius)
	{

		AircraftBearing = Bearing(WayPointList[Task[ActiveWayPoint].Index].Lattitude,
																WayPointList[Task[ActiveWayPoint].Index].Longditude,
																Basic->Lattitude ,
																Basic->Longditude);

		if(Task[ActiveWayPoint].AATStartRadial < Task[ActiveWayPoint].AATFinishRadial )
		{
			if(
					(AircraftBearing > Task[ActiveWayPoint].AATStartRadial)
					&&
					(AircraftBearing < Task[ActiveWayPoint].AATFinishRadial)
				)
				return TRUE;
		}

		if(Task[ActiveWayPoint].AATStartRadial > Task[ActiveWayPoint].AATFinishRadial )
		{
			if(
					(AircraftBearing > Task[ActiveWayPoint].AATStartRadial)
					||
					(AircraftBearing < Task[ActiveWayPoint].AATFinishRadial)
				)
				return TRUE;
		}
	}
	return FALSE;
}


int InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static int InSector = FALSE;
	double AircraftBearing;
	double FirstPointDistance;

	// No Task Loaded
	if(Task[0].Index == -1)
	{
		return FALSE;
	}


	FirstPointDistance = Distance(Basic->Lattitude ,Basic->Longditude ,WayPointList[Task[0].Index].Lattitude , WayPointList[Task[0].Index].Longditude);

	if(!StartLine) // Start Circle
	{
		if(FirstPointDistance< StartRadius)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	// Start Line
	AircraftBearing = Bearing(WayPointList[Task[0].Index].Lattitude,
															WayPointList[Task[0].Index].Longditude,
															Basic->Lattitude ,
															Basic->Longditude);

	AircraftBearing = AircraftBearing - Task[0].Bisector ;

	if( (AircraftBearing >= -90) && (AircraftBearing <= 90))
	{
		if(FirstPointDistance < StartRadius)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static BOOL StartSectorEntered = FALSE;

	if(AATEnabled)
		return;

	if(ActiveWayPoint == 0)
	{
		if(InStartSector(Basic,Calculated))
		{
			StartSectorEntered = TRUE;
		}
		else
		{
			if(StartSectorEntered == TRUE)
			{
				if(ActiveWayPoint < MAXTASKPOINTS)
				{
					if(Task[ActiveWayPoint+1].Index >= 0)
					{
						ActiveWayPoint ++;
						Calculated->TaskStartTime = Basic->Time ;
						Calculated->LegStartTime = Basic->Time;
						StartSectorEntered = FALSE;
					}
				}
			}
		}
	}
	else if(ActiveWayPoint >0)
	{
		if(InStartSector(Basic, Calculated))
		{
			if(Basic->Time - Calculated->TaskStartTime < 600)
			{
				ActiveWayPoint = 0;
				StartSectorEntered = TRUE;
			}
		}
		if(InTurnSector(Basic,Calculated))
		{
			if(ActiveWayPoint < MAXTASKPOINTS)
			{
				if(Task[ActiveWayPoint+1].Index >= 0)
				{
					Calculated->LegStartTime = Basic->Time;
					ActiveWayPoint ++;
					return;
				}
			}
		}
	}
}

void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static BOOL StartSectorEntered = FALSE;

	if(!AATEnabled)
		return;

	if(ActiveWayPoint == 0)
	{
		if(InStartSector(Basic,Calculated))
		{
			StartSectorEntered = TRUE;
		}
		else
		{
			if(StartSectorEntered == TRUE)
			{
				if(ActiveWayPoint < MAXTASKPOINTS)
				{
					if(Task[ActiveWayPoint+1].Index >= 0)
					{
						ActiveWayPoint ++;
						Calculated->TaskStartTime = Basic->Time ;
						Calculated->LegStartTime = Basic->Time;
						StartSectorEntered = FALSE;
					}
				}
			}
		}
	}
	else if(ActiveWayPoint >0)
	{
		if(InStartSector(Basic, Calculated))
		{
			if(Basic->Time - Calculated->TaskStartTime < 600)
			{
				ActiveWayPoint = 0;
				StartSectorEntered = TRUE;
			}
		}
		if(InAATTurnSector(Basic,Calculated))
		{
			if(ActiveWayPoint < MAXTASKPOINTS)
			{
				if(Task[ActiveWayPoint+1].Index >= 0)
				{
					Calculated->LegStartTime = Basic->Time;
					ActiveWayPoint ++;
					return;
				}
			}
		}
	}
}


double GetTerrainHeight(double Lattitude, double Longditude)
{
	long SeekPos;
	static long LastSeek;
	static __int16 Alt = 0;
	__int16 NewAlt = 0;
	double X,Y;
	DWORD dwBytesRead;
	DWORD SeekRes, dwError;

	if(hTerrain == NULL)
		return 0;

	if(Lattitude > TerrainInfo.Top )
		return 0;
	if(Lattitude < TerrainInfo.Bottom )
		return 0;
	if(Longditude < TerrainInfo.Left )
		return 0;
	if(Longditude > TerrainInfo.Right )
		return 0;


	X =  Longditude -TerrainInfo.Left;
	X = X / TerrainInfo.StepSize ;

	X = (long)X;

	if(X<0)
		return 0;

	Y = TerrainInfo.Top  - Lattitude ;
	Y = Y / TerrainInfo.StepSize ;

	Y = (long)Y;

	if(Y<0)
		return 0;

	Y *= TerrainInfo.Columns;
	Y +=  X;

	SeekPos = (long)Y;
	SeekPos *= 2;
	SeekPos += sizeof(TERRAIN_INFO);

	if(SeekPos != LastSeek)
	{
		SeekRes = SetFilePointer(hTerrain,SeekPos,NULL,FILE_BEGIN);
		if(SeekRes == 0xFFFFFFFF && (dwError = GetLastError()) != NO_ERROR )

		{
			return 0;
		}
		ReadFile(hTerrain,&NewAlt,sizeof(__int16),&dwBytesRead,NULL);

		Alt = NewAlt;
		LastSeek = SeekPos;
	}

	if(Alt<0) Alt = 0;
	return (double)Alt;
}

static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	double Alt = 0;

	Alt = GetTerrainHeight(Basic->Lattitude , Basic->Longditude );

	if(Alt<0) Alt = 0;

	Calculated->TerrainAlt = Alt;
	Calculated->AltitudeAGL = Basic->Altitude - Calculated->TerrainAlt;
}


extern TCHAR szRegistryTerrainFile[];

void OpenTerrain(void)
{
	DWORD dwBytesRead;
	static TCHAR	szFile[MAX_PATH] = TEXT("\0");

	GetRegistryString(szRegistryTerrainFile, szFile, MAX_PATH);

	hTerrain = NULL;
	hTerrain = CreateFile(szFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if( hTerrain == NULL)
	{
		return;
	}
	ReadFile(hTerrain,&TerrainInfo,sizeof(TERRAIN_INFO),&dwBytesRead,NULL);

}



void CloseTerrain(void)
{
	if( hTerrain == NULL)
	{
		return;
	}
	else
	{
		CloseHandle(hTerrain);
	}
}

void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	int i;
	double LegCovered, LegToGo, LegDistance, LegBearing, LegAltitude;
	double TaskAltitudeRequired = 0;


	// Calculate Task Distances
	if(ActiveWayPoint >=1)
	{
		LegDistance = Distance(WayPointList[Task[ActiveWayPoint].Index].Lattitude,
													 WayPointList[Task[ActiveWayPoint].Index].Longditude,
													 WayPointList[Task[ActiveWayPoint-1].Index].Lattitude,
													 WayPointList[Task[ActiveWayPoint-1].Index].Longditude);

		LegToGo = Distance(Basic->Lattitude , Basic->Longditude ,
													WayPointList[Task[ActiveWayPoint].Index].Lattitude,
													WayPointList[Task[ActiveWayPoint].Index].Longditude);

		LegCovered = LegDistance - LegToGo;

		if(LegCovered <=0)
			Calculated->TaskDistanceCovered = 0;
		else
			Calculated->TaskDistanceCovered = LegCovered;

		Calculated->LegDistanceToGo = LegToGo;
		Calculated->LegDistanceCovered = Calculated->TaskDistanceCovered;

		if(Basic->Time != Calculated->LegStartTime)
			Calculated->LegSpeed = Calculated->LegDistanceCovered / (Basic->Time - Calculated->LegStartTime);


		for(i=0;i<ActiveWayPoint-1;i++)
		{
			LegDistance = Distance(WayPointList[Task[i].Index].Lattitude,
														 WayPointList[Task[i].Index].Longditude,
														 WayPointList[Task[i+1].Index].Lattitude,
														 WayPointList[Task[i+1].Index].Longditude);

			Calculated->TaskDistanceCovered += LegDistance;

		}

		if(Basic->Time != Calculated->TaskStartTime)
			Calculated->TaskSpeed = Calculated->TaskDistanceCovered / (Basic->Time - Calculated->TaskStartTime);
	}

	// Calculate Final Glide To Finish
	Calculated->TaskDistanceToGo = 0;
	if(ActiveWayPoint >=0)
	{
		i=ActiveWayPoint;

		LegBearing = Bearing(Basic->Lattitude , Basic->Longditude ,
													WayPointList[Task[i].Index].Lattitude,
													WayPointList[Task[i].Index].Longditude);

		LegToGo = Distance(Basic->Lattitude , Basic->Longditude ,
													WayPointList[Task[i].Index].Lattitude,
													WayPointList[Task[i].Index].Longditude);

		LegAltitude = McReadyAltitude(MACREADY/LIFTMODIFY, LegToGo, LegBearing, Calculated->WindSpeed, Calculated->WindBearing);
		LegAltitude = LegAltitude * (1/BUGS);

		TaskAltitudeRequired = LegAltitude;
		Calculated->TaskDistanceToGo = LegToGo;

		i++;
		while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
		{
			LegDistance = Distance(WayPointList[Task[i].Index].Lattitude,
													 WayPointList[Task[i].Index].Longditude,
													 WayPointList[Task[i-1].Index].Lattitude,
													 WayPointList[Task[i-1].Index].Longditude);

			LegBearing = Bearing(WayPointList[Task[i-1].Index].Lattitude,
													 WayPointList[Task[i-1].Index].Longditude,
													 WayPointList[Task[i].Index].Lattitude,
													WayPointList[Task[i].Index].Longditude);


			LegAltitude = McReadyAltitude(MACREADY/LIFTMODIFY, LegDistance, LegBearing, Calculated->WindSpeed, Calculated->WindBearing);
			LegAltitude = LegAltitude * (1/BUGS);

			TaskAltitudeRequired += LegAltitude;

			Calculated->TaskDistanceToGo += LegDistance;

			i++;
		}
		Calculated->TaskAltitudeRequired = TaskAltitudeRequired + SAFTEYALTITUDE;
		Calculated->TaskAltitudeDifference = Basic->Altitude - (Calculated->TaskAltitudeRequired + WayPointList[Task[i-1].Index].Altitude);

		if(  (Basic->Altitude - WayPointList[Task[i-1].Index].Altitude) > 0)
		{
			Calculated->LDFinish = Calculated->TaskDistanceToGo / (Basic->Altitude - WayPointList[Task[i-1].Index].Altitude)  ;
		}
		else
		{
			Calculated->LDFinish = 9999;
		}
	}
}

void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	static BOOL BelowGlide = TRUE;

	if(BelowGlide == TRUE)
	{
		if(Calculated->TaskAltitudeDifference > 10)
		{
			BelowGlide = FALSE;
			sndPlaySound(TEXT("My Documents\\FinalGlide.wav"),SND_ASYNC|SND_NODEFAULT);
		}
	}
	else
	{
		if(Calculated->TaskAltitudeDifference < 10)
		{
			BelowGlide = TRUE;
			sndPlaySound(TEXT("My Documents\\Tiptoe.wav"),SND_ASYNC|SND_NODEFAULT);
		}
	}
}
extern int AIRSPACEWARNINGS;
extern int WarningTime;

void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	if(Calculated->Circling)
	{
		Calculated->NextLattitude = GPS_INFO.Lattitude;
		Calculated->NextLongditde = GPS_INFO.Longditude;
		Calculated->NextAltitude = Basic->Altitude + Calculated->Average30s * 30;
	}
	else
	{
		Calculated->NextLattitude = FindLattitude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed*WarningTime );
		Calculated->NextLongditde = FindLongditude(GPS_INFO.Lattitude, GPS_INFO.Longditude, GPS_INFO.TrackBearing, GPS_INFO.Speed*WarningTime);
		Calculated->NextAltitude = Basic->Altitude + Calculated->Average30s * WarningTime;
	}
}

int LastCi =-1;
int LastAi =-1;
HWND hCMessage = NULL;
HWND hAMessage = NULL;

void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	int i;
	TCHAR szMessageBuffer[1024];
	TCHAR szTitleBuffer[1024];

	if(!AIRSPACEWARNINGS)
	{
		if(hCMessage)
		{
			LastCi = -1;LastAi = -1;
			DestroyWindow(hCMessage);
			hCMessage = NULL;
		}
		if(hAMessage)
		{
			DestroyWindow(hAMessage);
			hAMessage = NULL;
		}
		return;
	}

	i= FindAirspaceCircle(Calculated->NextLongditde, Calculated->NextLattitude );
	if(i != -1)
	{
		if(i == LastCi)
		{
			return;
		}

		if(hCMessage)
		{
			DestroyWindow(hCMessage);
			hCMessage = NULL;
		}

		MessageBeep(MB_ICONEXCLAMATION);
		FormatWarningString(AirspaceCircle[i].Type , AirspaceCircle[i].Name , AirspaceCircle[i].Base, AirspaceCircle[i].Top, szMessageBuffer, szTitleBuffer );
		if(
				(DisplayOrientation == TRACKUP)
				||
				((DisplayOrientation == NORTHCIRCLE) && (Calculated->Circling == FALSE) )
			)
			hCMessage = CreateWindow(TEXT("EDIT"),szMessageBuffer,WS_VISIBLE|WS_CHILD|ES_MULTILINE |ES_CENTER|WS_BORDER,
															0, 0,240,50,hWndMapWindow,NULL,hInst,NULL);
		else
			hCMessage = CreateWindow(TEXT("EDIT"),szMessageBuffer,WS_VISIBLE|WS_CHILD|ES_MULTILINE |ES_CENTER|WS_BORDER,
															0, 180,240,50,hWndMapWindow,NULL,hInst,NULL);

		ShowWindow(hCMessage,SW_SHOW);
		UpdateWindow(hCMessage);
		LastCi = i;
		return;
	}
	else if(hCMessage)
	{
		DestroyWindow(hCMessage);
		hCMessage = NULL;
		LastCi = -1;
	}
	else
	{
		LastCi = -1;
	}


	i= FindAirspaceArea(Calculated->NextLongditde,Calculated->NextLattitude);
	if(i != -1)
	{
		if(i == LastAi)
		{
			return;
		}

		if(hAMessage)
		{
			DestroyWindow(hAMessage);
			hAMessage = NULL;
		}

		MessageBeep(MB_ICONEXCLAMATION);
		FormatWarningString(AirspaceArea[i].Type , AirspaceArea[i].Name , AirspaceArea[i].Base, AirspaceArea[i].Top, szMessageBuffer, szTitleBuffer );
		if(
			(DisplayOrientation == TRACKUP)
			||
			((DisplayOrientation == NORTHCIRCLE) && (Calculated->Circling == FALSE) )
			)

			hAMessage = CreateWindow(TEXT("EDIT"),szMessageBuffer,WS_VISIBLE|WS_CHILD|ES_MULTILINE |ES_CENTER|WS_BORDER,
																0, 0,240,50,hWndMapWindow,NULL,hInst,NULL);
		else
			hAMessage = CreateWindow(TEXT("EDIT"),szMessageBuffer,WS_VISIBLE|WS_CHILD|ES_MULTILINE |ES_CENTER|WS_BORDER,
																0, 180,240,50,hWndMapWindow,NULL,hInst,NULL);

		ShowWindow(hAMessage,SW_SHOW);
		UpdateWindow(hAMessage);

		LastAi = i;
		return;
	}
	else if(hAMessage)
	{
		DestroyWindow(hAMessage);
		hAMessage = NULL;
		LastAi = -1;
	}
	else
	{
		LastAi = -1;
	}
}

void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	double Temp;
	int i;
	double MaxDistance, MinDistance;
	double LegToGo, LegDistance;
	double TaskAltitudeRequired = 0;

	if(!AATEnabled)
	{
		return;
	}

	Temp = Basic->Time - Calculated->TaskStartTime;
	if((Temp >=0)&&(ActiveWayPoint >0))
	{
		Calculated->AATTimeToGo = (AATTaskLength*60) - Temp;
		if(Calculated->AATTimeToGo <= 0)
			Calculated->AATTimeToGo = 0;
		if(Calculated->AATTimeToGo >= (AATTaskLength * 60) )
			Calculated->AATTimeToGo = (AATTaskLength * 60);
	}

	MaxDistance = 0; MinDistance = 0;
	// Calculate Task Distances

	Calculated->TaskDistanceToGo = 0;
	if(ActiveWayPoint >=0)
	{
		i=ActiveWayPoint;

		LegToGo = Distance(Basic->Lattitude , Basic->Longditude ,
													WayPointList[Task[i].Index].Lattitude,
													WayPointList[Task[i].Index].Longditude);

		if(Task[ActiveWayPoint].AATType == CIRCLE)
		{
			MaxDistance = LegToGo + (Task[i].AATCircleRadius * 2);
			MinDistance = LegToGo - (Task[i].AATCircleRadius * 2);
		}
		else
		{
			MaxDistance = LegToGo + (Task[ActiveWayPoint].AATSectorRadius * 2);
			MinDistance = LegToGo;
		}

		i++;
		while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
		{
			LegDistance = Distance(WayPointList[Task[i].Index].Lattitude,
													 WayPointList[Task[i].Index].Longditude,
													 WayPointList[Task[i-1].Index].Lattitude,
													 WayPointList[Task[i-1].Index].Longditude);

			if(Task[ActiveWayPoint].AATType == CIRCLE)
			{
				MaxDistance += LegDistance + (Task[i].AATCircleRadius * 2);
				MinDistance += LegDistance- (Task[i].AATCircleRadius * 2);
			}
			else
			{
				MaxDistance += LegDistance + (Task[ActiveWayPoint].AATSectorRadius * 2);
				MinDistance += LegDistance;
			}
			i++;
		}
		Calculated->AATMaxDistance = MaxDistance;
		Calculated->AATMinDistance = MinDistance;
		if(Calculated->AATTimeToGo >0)
		{
			Calculated->AATMaxSpeed = Calculated->AATMaxDistance / Calculated->AATTimeToGo;
			Calculated->AATMinSpeed = Calculated->AATMinDistance / Calculated->AATTimeToGo;
		}
	}
}










