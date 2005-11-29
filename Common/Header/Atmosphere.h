#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H


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
  float thermalIndex; 
  void updateTemps(float rh, float t);
  void updateThermalIndex(unsigned short level, bool newdata=true);
  int nmeasurements;

  float thermalHeight; // as estimated by this level
  float cloudBase; // as estimated by this level
};


#define CUSONDE_HEIGHTSTEP 100 // meters between levels
#define CUSONDE_NUMLEVELS 100 // number of levels
#define DALR -0.00974 // degrees C per meter
#define TITHRESHOLD -1.6 // thermal index threshold in degrees C

class CuSonde {
public:
  static float maxGroundTemperature;
  static unsigned short last_level; 
  static void updateMeasurements(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
  static CuSondeLevel cslevels[CUSONDE_NUMLEVELS];
  static void findCloudBase(unsigned short level);
  static void findThermalHeight(unsigned short level);
  static void adjustForecastTemperature(float delta);

  static float thermalHeight; // as estimated by this level
  static float cloudBase; // as estimated by this level

  static void test();
};

#endif
