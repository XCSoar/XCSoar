#pragma once

#include <map>
#include "parser.h"
#include "ClimbAverageCalculator.h"

class FlarmCalculations
{
public:
	FlarmCalculations(void);
	~FlarmCalculations(void);

	double Average30s(long flarmId, double curTime, double curAltitude);

private:
	typedef std::map<long, ClimbAverageCalculator*> AverageCalculatorMap;
	AverageCalculatorMap averageCalculatorMap;
};
