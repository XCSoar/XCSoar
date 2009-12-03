/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef ONLINECONTEST_H
#define ONLINECONTEST_H

#define MAX_OLC_POINTS 300
#define MATSIZE (MAX_OLC_POINTS+1)*(MAX_OLC_POINTS/2) // for even MAX_OLC_POINTS

#include "Thread/Mutex.hpp"
#include "GeoPoint.hpp"

struct SETTINGS_COMPUTER;

typedef struct _OLCSolution
{
  GEOPOINT location[7];
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

  GEOPOINT locpnts[MAX_OLC_POINTS];
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

  double getDt(const SETTINGS_COMPUTER &settings) const;
  double getD(const SETTINGS_COMPUTER &settings) const;
  double getValid(const SETTINGS_COMPUTER &settings) const;
  double getScore(const SETTINGS_COMPUTER &settings) const;
  double getFinished(const SETTINGS_COMPUTER &settings) const;

private:

  void Clear();

private:

  //  int sindex(int y, int x);

  unsigned short dmval[MATSIZE];
  int indexval[MAX_OLC_POINTS];
  int maxdist; /* maximale Distanz in Metern zwischen zwei Punkten */

  int istart;

  void UpdateSolution(int dbest, int tbest,
		      int p1, int p2, int p3, int p4, int p5, int p6, int p7,
		      OLCSolution* solution, double score, bool finished);
public:

  GEOPOINT loc_proj;

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
  bool addPoint(const GEOPOINT &loc, double alt, double time, double bearing,
		const SETTINGS_COMPUTER &settings);

public:
  bool Optimize(const SETTINGS_COMPUTER &settings, bool isflying);
  int getN() const;
  const GEOPOINT &getLocation(int i) const;
  double getTime(int i) const;
  double getAltitudeHigh(int i) const;

private:

  void thin_data();

  int optimize_internal(const SETTINGS_COMPUTER &settings);
  int triangle_legal(int i2, int i3, int i4, int i5);
  int scan_triangle(const SETTINGS_COMPUTER &settings);
  int scan_sprint(const SETTINGS_COMPUTER &settings);
  int scan_sprint_inprogress(const SETTINGS_COMPUTER &settings);
  int scan_sprint_finished(const SETTINGS_COMPUTER &settings);
  int scan_classic(const SETTINGS_COMPUTER &settings);
  Mutex mutexOLC;
 public:
  void Lock() { mutexOLC.Lock(); }
  void Unlock() { mutexOLC.Unlock(); }
};

#endif /* ONLINECONTEST_H */
