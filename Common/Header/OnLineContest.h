#ifndef ONLINECONTEST_H
#define ONLINECONTEST_H

typedef struct _OLCSolution
{
  double latitude[5];
  double longitude[5];
  bool valid;
  double distance;
  int time;
} OLCSolution;


class OLCOptimizer {
public:
  OLCOptimizer();
  ~OLCOptimizer();
  void ResetFlight();

private:

  void Clear();

private:
  int pnts;
  int pnts_offset;
  int pnts_in;
  int pnts_offset_in;
  int* dmval;
  int maxdist; /* maximale Distanz in Metern zwischen zwei Punkten */

  /*
   *	Indexe der 5 besten Punkte f?r:
               freie Strecke,
	       FAI-Dreieck,
	       flaches Dreieck
   */
  int max1, max2, max3, max4, max5;
  int max1fai, max2fai, max3fai, max4fai, max5fai;
  int max1flach, max2flach, max3flach, max4flach, max5flach;
  int maxroute, bestfai, bestflach;
  int i2;

  void UpdateSolution(int dbest,
		      int p1, int p2, int p3, int p4, int p5,
		      OLCSolution* solution);
public:
  bool stop;

  OLCSolution solution_FAI_triangle;
  OLCSolution solution_FAI_sprint;

private:
  int* dmin;
  int* dmini;
  int* dminj;
  double* latpnts;
  double* lonpnts;
  int* altpnts;
  long* timepnts;

  double lat_proj;
  double lon_proj;
  int alt_proj;
  bool project;

  int* maxenddist;
  int* maxendpunkt;

  int initdmval();

  int initdmin();
  int initmaxend();

public:
  void addPoint(double lon, double lat, double alt, double time);

public:
  bool Optimize();
  bool OptimizeProjection(double lon, double lat, double alt);
  int getN();
  double getLatitude(int i);
  double getLongitude(int i);

private:
  int optimize_internal();
};

#endif ONLINECONTEST_H
