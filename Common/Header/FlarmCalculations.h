#ifndef FLARMCALCULATIONS_H
#define FLARMCALCULATIONS_H

#ifdef FLARM_AVERAGE

#include "NMEA/Info.h"

#include <map>
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

#endif
#endif
