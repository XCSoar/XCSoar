#include "stdafx.h"
#include "TeamCodeCalculation.h"
#include "math.h"


void ConvertHeadingToTeamCode(double heading, TCHAR *code);
void NumberToTeamCode(double value, TCHAR *code, int minCiffers);
double GetBearing(TCHAR *code );
double GetRange(TCHAR *code );
int GetValueFromTeamCode(TCHAR *code, int maxCount);

#define TEAMCODE_COMBINAIONS 1296
#define DEG_TO_RAD .0174532925199432958

void GetTeamCode(TCHAR *code, double bearing, double range)
{
	memset(code, 0, sizeof(TCHAR) * 10);

		// trim to seeyou
	//double trim = cos(bearing*DEG_TO_RAD)*range/8500;
		//bearing+= 1.2;

	 //   if (bearing > 360)
		//{
		//	bearing -= 360;
		//}
		


	ConvertHeadingToTeamCode(bearing, code);	
	NumberToTeamCode(range/100.0, &code[2],0);
}

void ConvertHeadingToTeamCode(double heading, TCHAR *code)
{
	if (heading >= 360)
	{
		code[0] = '-';
		code[1] = '-';
		return;
	}	 

	double bamValue = (heading * TEAMCODE_COMBINAIONS) / 360.0;	
	NumberToTeamCode(bamValue, code, 2);
}

void NumberToTeamCode(double value, TCHAR *code, int minCiffers)
{

	int maxCif = 0;
	int curCif = 0;

	if (minCiffers > 0)
	{
		maxCif = minCiffers - 1;
		curCif = maxCif;
	}

	double rest = value;
	while(rest > 0 || curCif >= 0 )
	{
		int cifVal = (int)pow(36.0, curCif);
		int partSize = (int)(rest / cifVal);
		int partVal = partSize * cifVal;
		int txtPos = maxCif - curCif;

		if (partSize < 10)
		{		
			rest -= partVal;
			code[txtPos] = (unsigned char)('0' + partSize);
			curCif--;
		}
		else if (partSize < 36)
		{
			rest -= partVal;
			code[txtPos] = (unsigned char)('A' + partSize - 10);
			curCif--;
		}
		else
		{
			curCif++;
			maxCif = curCif;
		}

		if (rest < 1) rest = 0;
	}
}

double GetBearing(TCHAR *code )
{		
	int val = GetValueFromTeamCode(code, 2);
	
	double bearing = (val * 360.0/TEAMCODE_COMBINAIONS);
	 bearing -= 0;
	if (bearing < 0)
	{
		bearing += 360;
	}

	return bearing;
}

double GetRange(TCHAR *code )
{
	int val = GetValueFromTeamCode(&code[2], 3);
	return val*100.0;
}

double GetTeammateBearingFromRef(TCHAR *code )
{
	return GetBearing(code);
}

double GetTeammateRangeFromRef(TCHAR *code )
{
	return GetRange(code);
}

int GetValueFromTeamCode(TCHAR *code, int maxCount)
{
	int val = 0;
	int charPos = 0;
	while (code[charPos] != 0 && charPos < maxCount)
	{		
		int cifferVal = 0;
		if (code[charPos] >= '0' && code[charPos] <= '9')
		{
			cifferVal = (int) (code[charPos] - '0');
		}
		else if (code[charPos] >= 'A' && code[charPos] <= 'Z')
		{
			cifferVal = (int) (code[charPos]+ - 'A')+10;
		}

		val = val * 36;
		val += (int)cifferVal;

		charPos++;
	}	
	return val;
}

void CalcTeamMatePos(double ownBear, double ownDist, double mateBear, double mateDist, double *bearToMate, double *distToMate)
{
	// define constants
	double PI = 3.14159265358979;
	double toRad = PI / 180.0;
	double toDeg = 180.0 / PI;

	// convert bearings to radians
	ownBear = ownBear * toRad; 
	mateBear = mateBear * toRad; 

	// Calculate range
	double Xs = ownDist*sin(ownBear) - mateDist*sin(mateBear);
	double Ys = ownDist*cos(ownBear) - mateDist*cos(mateBear);
	double range = sqrt((Xs*Xs)+(Ys*Ys));
	*distToMate = range;

	// Calculate bearing
	double bearing;
	
	if (Xs != 0)
	{
		bearing = (atan(Ys/Xs) * toDeg);
		if (Xs < 0) bearing =  bearing + 180;
		*bearToMate = 90.0 - bearing;
		if (*bearToMate < 0) *bearToMate += 360;
	}
	else
	{
		if (Ys >= 0)
		{
			*bearToMate = 180;
		}
		else
		{
			*bearToMate = 0;
		}
	}
}

void CalcTeammateBearingRange(double ownBear, double ownDist, TCHAR *TeamMateCode,  double *bearToMate, double *distToMate)
{
	double calcBearing = GetBearing(TeamMateCode)/*+ 180*/;
	double calcRange = GetRange(TeamMateCode);	


	//if (calcBearing > 360)
	//{
	//	calcBearing -= 360;
	//}
	

	CalcTeamMatePos(ownBear, ownDist, calcBearing, calcRange, bearToMate, distToMate);
}
