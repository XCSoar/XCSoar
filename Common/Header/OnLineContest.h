#ifndef ONLINECONTEST_H
#define ONLINECONTEST_H

#define MAX_OLC_POINTS 300
#define MATSIZE (MAX_OLC_POINTS+1)*(MAX_OLC_POINTS/2) // for even MAX_OLC_POINTS

typedef struct _OLCSolution
{
  double latitude[7];
  double longitude[7];
  bool valid;
  double distance;
  int time;
  double score;
  bool finished;
} OLCSolution;

class OLCData {
 public:

  OLCSolution solution_FAI_triangle;
  OLCSolution solution_FAI_sprint;
  OLCSolution solution_FAI_classic;

  double latpnts[MAX_OLC_POINTS];
  double lonpnts[MAX_OLC_POINTS];
  int altpntslow[MAX_OLC_POINTS];
  int altpntshigh[MAX_OLC_POINTS];
  long timepnts[MAX_OLC_POINTS];

  int altminimum;
  long tsprintstart;
  int pnts_in;
  double waypointbearing;
  double distancethreshold;

};

class OLCOptimizer {
public:
  OLCOptimizer();
  virtual ~OLCOptimizer();
  void ResetFlight();

private:

  void Clear();

private:

  //  int sindex(int y, int x);

  int pnts;
  unsigned short dmval[MATSIZE];
  int indexval[MAX_OLC_POINTS];
  int maxdist; /* maximale Distanz in Metern zwischen zwei Punkten */

  int istart;

  void UpdateSolution(int dbest, int tbest,
		      int p1, int p2, int p3, int p4, int p5, int p6, int p7,
		      OLCSolution* solution, double score, bool finished);
public:

  double lat_proj;
  double lon_proj;

  bool stop;

  OLCData data;

private:

  bool flying;

  unsigned short isplit[MATSIZE];
  unsigned short ibestendclassic[MATSIZE];
  unsigned short istartsprint[MAX_OLC_POINTS];

  int initdmval();
  int initisplit();
  int initibestend();
  int initistartsprint();
  bool busy;

public:
  bool addPoint(double lon, double lat, double alt, double time, double bearing);

public:
  void SetLine();
  bool Optimize(bool isflying);
  int getN();
  double getLatitude(int i);
  double getLongitude(int i);

private:

  void thin_data();

  int optimize_internal();
  int triangle_legal(int i2, int i3, int i4, int i5);
  int scan_triangle();
  int scan_sprint();
  int scan_sprint_inprogress();
  int scan_sprint_finished();
  int scan_classic();
};

#endif ONLINECONTEST_H
