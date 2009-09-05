/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#if !defined(AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#define MAXSAFETYSPEED 200

#include "Sizes.h"

extern unsigned POLARID;
extern double POLAR[POLARSIZE];
extern double WEIGHTS[POLARSIZE];

class GlidePolar {
 public:

  static double MacCreadyAltitude(double MCREADY, double Distance,
                                  const double Bearing,
				  const double WindSpeed,
                                  const double WindBearing,
				  double *BestCruiseTrack,
                                  double *VMacCready,
				  const bool isFinalGlide,
                                  double *timetogo,
                                  double AltitudeAboveTarget=1.0e6,
				  double cruise_efficiency=1.0);

  static double MacCreadyRisk(double HeightAboveTerrain, double MaxThermalHeight,
                              double MCREADY);

  static double GetAUW();

  static double AbortSafetyMacCready();
  static double SafetyMacCready;
  static bool AbortSafetyUseCurrent;

  //  static double BallastFactor;
  static double RiskGamma;
  static double polar_a;
  static double polar_b;
  static double polar_c;
  static int    Vminsink;
  static int    Vbestld;
  static double bestld;
  static double minsink;
  static double WingArea;
  static double WingLoading;

  static double sinkratecache[MAXSAFETYSPEED];

  static double SinkRate(double Vias);
  static double SinkRate(double Vias,
                double loadfactor);
  static double SinkRate(double a,double b, double c,
                         double MC, double HW, double V);
  static double FindSpeedForSinkRate(double w);
  static double SinkRateFast(const double &MC, const int &v);
 private:
  static double _SinkRateFast(const double &MC, const int &v);
  static double MacCreadyAltitude_internal(double MCREADY,
                                           double Distance,
                                           const double Bearing,
                                           const double WindSpeed,
                                           const double WindBearing,
                                           double *BestCruiseTrack,
                                           double *VMacCready,
                                           const bool isFinalGlide,
                                           double *timetogo,
					   const double cruise_efficiency);

  static double MacCreadyAltitude_heightadjust(double MCREADY,
                                               double Distance,
                                               const double Bearing,
                                               const double WindSpeed,
                                               const double WindBearing,
                                               double *BestCruiseTrack,
                                               double *VMacCready,
                                               const bool isFinalGlide,
                                               double *timetogo,
                                               const double AltitudeAboveTarget,
					       const double cruise_efficiency);

 private:
  static double MACCREADY; // m/s
  static double BUGS;   
  static double BALLAST;
  static void Lock();
  static void Unlock();
  static double BallastLitres;
  static double CRUISE_EFFICIENCY;
 public:
  static double GetMacCready();
  static double GetBugs();
  static double GetBallast();
  static double GetBallastLitres();
  static double GetCruiseEfficiency();

  static void SetCruiseEfficiency(double);
  static void SetMacCready(double);
  static void SetBallast(double);
  static void SetBugs(double);
  static void UpdatePolar(bool send);
};

#endif
