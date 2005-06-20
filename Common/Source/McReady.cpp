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
#include "McReady.h"
#include "resource.h"
#include "externs.h"

#include "XCSoar.h"

#include <math.h>
#include <windows.h>
/*
double a = -0.00190772449;
double b = 0.06724332;
double c = -1.141438761599;
*/

static double SinkRate(double a,double b, double c, double MC, double HW, double V);


double McReadyAltitude(double MCREADY, double Distance, double Bearing, double WindSpeed, double WindBearing)
{
	int i,j;
	double BestSpeed, BestGlide, Glide;
	double BestSinkRate, TimeToDest;
	double AltitudeNeeded;
	double HeadWind, CrossWind;
	double CrossBearing;
	double VMG;

	double a,b,c;
	double BallastWeight;

	BallastWeight = WEIGHTS[2] * BALLAST;
	BallastWeight += WEIGHTS[0] + WEIGHTS[1];
	BallastWeight = (double)sqrt(BallastWeight);

	a = POLAR[0] / BallastWeight;
	b = POLAR[1];
	c = POLAR[2] * BallastWeight;

	CrossBearing = Bearing - WindBearing;
	CrossBearing = (double)(pi*CrossBearing)/180;

	HeadWind = WindSpeed * (double)cos(CrossBearing);
	CrossWind = WindSpeed * (double)sin(CrossBearing);


	//Calculate Best Glide Speed
	for(j=0;j<6;j++)
	{
		BestSpeed = 2;
		BestGlide = -BestSpeed / SinkRate(a,b,c,MCREADY,HeadWind,2);

		for(i=4;i<100;i+=2)
		{
			Glide = (double)(-i)/ SinkRate(a,b,c,MCREADY,HeadWind,(double)i);
			if(Glide >= BestGlide)
			{
				BestGlide = Glide;
				BestSpeed = (double)i;
			}
			else
			{
				break;
			}
		}

		BestSpeed += HeadWind;
		// Calculate Sink Rate @ Best Glide

		if(fabs(CrossWind) > BestSpeed)
		{
			HeadWind = BestSpeed;
			VMG = (double)0.00001;
		}
		else
		{
			VMG = (double)sqrt(BestSpeed*BestSpeed - CrossWind*CrossWind);
			VMG -= HeadWind;
			HeadWind = BestSpeed - VMG;
		}
	}

	BestSinkRate = SinkRate(a,b,c,0,0,BestSpeed);
	TimeToDest = Distance / (VMG);
	AltitudeNeeded = -BestSinkRate * TimeToDest;

	return AltitudeNeeded;
}

double SinkRate(double a,double b, double c, double MC, double HW, double V)
{
	double temp;

	temp =  a*(V+HW)*(V+HW);
	temp += b*(V+HW);
	temp += c-MC;

	return temp;
}
