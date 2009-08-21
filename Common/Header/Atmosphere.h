#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "NMEA/Info.h"
#include "Calculations.h"

class CuSondeLevel {
public:
  CuSondeLevel() {
    nmeasurements = 0;
    thermalHeight = -1;
    cloudBase = -1;
  }
  double airTemp; // degrees C
  double dewpoint; // degrees C
  double tempDry; // degrees C
  double thermalIndex;
  void updateTemps(double rh, double t);
  void updateThermalIndex(unsigned short level, bool newdata=true);
  int nmeasurements;

  double thermalHeight; // as estimated by this level
  double cloudBase; // as estimated by this level
};


#define CUSONDE_HEIGHTSTEP 100 // meters between levels
#define CUSONDE_NUMLEVELS 100 // number of levels
#define DALR -0.00974 // degrees C per meter
#define TITHRESHOLD -1.6 // thermal index threshold in degrees C

class CuSonde {
public:
  static double maxGroundTemperature;
  static double hGround;
  static unsigned short last_level;
  static void updateMeasurements(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
  static CuSondeLevel cslevels[CUSONDE_NUMLEVELS];
  static void findCloudBase(unsigned short level);
  static void findThermalHeight(unsigned short level);
  static void adjustForecastTemperature(double delta);
  static void setForecastTemperature(double val);

  static double thermalHeight; // as estimated by this level
  static double cloudBase; // as estimated by this level

  static void test();
};

#endif
