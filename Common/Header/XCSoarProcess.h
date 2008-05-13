#ifndef PROCESS_H
#define PROCESS_H
#include "externs.h"

void				NoProcessing(int UpDown);
void				WindSpeedProcessing(int UpDown);
void				WindDirectionProcessing(int UpDown);
void				MacCreadyProcessing(int UpDown);
void				AccelerometerProcessing(int UpDown);
void				NextUpDown(int UpDown);
void				SpeedProcessing(int UpDown);
void				DirectionProcessing(int UpDown);
void				AltitudeProcessing(int UpDown);
void				AirspeedProcessing(int UpDown);
void				ForecastTemperatureProcessing(int UpDown);
int DetectStartTime(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
int DetectCurrentTime(void);
int TimeLocal(int d);

#endif
