#include "FlarmCalculations.h"

FlarmCalculations::FlarmCalculations(void)
{
}

FlarmCalculations::~FlarmCalculations(void)
{
}


double FlarmCalculations::Average30s(long flarmId, double curTime, double curAltitude)
{
	ClimbAverageCalculator *itemTemp = NULL;
	AverageCalculatorMap::iterator iterFind = averageCalculatorMap.find(flarmId);
	if( iterFind != averageCalculatorMap.end() )
	{
		itemTemp = averageCalculatorMap[flarmId];		
	}
	else
	{
		itemTemp = new ClimbAverageCalculator();
		 averageCalculatorMap[flarmId] = itemTemp;		
	}
	return itemTemp->GetAverage(curTime, curAltitude, 30);
}
