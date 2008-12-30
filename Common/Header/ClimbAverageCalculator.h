#ifndef CLIMBAVERAGECALCULATOR_H
#define CLIMBAVERAGECALCULATOR_H

#include <map>
#include "Parser.h"

class ClimbAverageCalculator
{
public:
	ClimbAverageCalculator(void);
	~ClimbAverageCalculator(void);
	double GetAverage(double curTime, double curAltitude, int averageTime);

private:
	static int const MAX_HISTORY = 40;
	struct flarmAltHistoryItem
	{
		double time;
		double altitude;
	};

	flarmAltHistoryItem history[MAX_HISTORY];   
  	int newestValIndex;
};

#endif

