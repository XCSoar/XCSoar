#if !defined(AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAXSAFETYSPEED 200

class GlidePolar {
 public:

  static double MacCreadyAltitude(double MCREADY, double Distance, 
                                  double Bearing, double WindSpeed, 
                                  double WindBearing, double *BestCruiseTrack, 
                                  double *VMacCready, bool isFinalGlide, 
                                  double *timetogo);


  static void SetBallast();
  static double GetAUW();

  static double AbortSafetyMacCready();
  static double SafetyMacCready;
  static bool AbortSafetyUseCurrent;

  //  static double BallastFactor;
  static double polar_a;
  static double polar_b;
  static double polar_c;
  static int Vminsink;
  static int Vbestld;
  static double bestld;
  static double minsink;
  static double BallastLitres;
  
  static double sinkratecache[MAXSAFETYSPEED];

  static double SinkRateFast(const double &MC, const int &v);
  static double SinkRate(double Vias);
  static double SinkRate(double Vias, 
                double loadfactor);
  static double SinkRate(double a,double b, double c, 
                         double MC, double HW, double V);
  static double FindSpeedForSinkRate(double w);
 private:
  static double MacCreadyAltitude_internal(double MCREADY, 
                                           double Distance, 
                                           double Bearing, 
                                           double WindSpeed, 
                                           double WindBearing, 
                                           double *BestCruiseTrack, 
                                           double *VMacCready, 
                                           bool isFinalGlide, 
                                           double *timetogo);

};

#endif
