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

#include "OnLineContest.h"
#include "Protection.hpp"
#include "Math/FastMath.h"
#include "McReady.h"
#include "Math/Earth.hpp"
#include "Units.hpp"

#include <math.h>


#define CONST_D_FAK 6371000.0
#define DISTANCETHRESHOLD 1000
#define DISTANCEUNITS 100


/*

memory requirements N*N*(int+int+int+int)

  300*300*(dmval 4, isplit 2, ibestendclassic 2)/2 = 360kb

  half for diagonal matrices: 0.7 meg
  half by using shorts? 0.35 meg

*/


OLCOptimizer::OLCOptimizer() {

  pnts = 0;
  data.pnts_in = 0;

  busy = false;

  loc_proj.Latitude = 0;
  loc_proj.Longitude = 0;

  flying = true;

  Clear();
  ResetFlight();
}

OLCOptimizer::~OLCOptimizer() {
  Clear();
  ResetFlight();
}


void OLCOptimizer::ResetFlight() {
  data.pnts_in = 0;
  pnts = 0;
  data.distancethreshold = DISTANCETHRESHOLD/2.0; // 500 meters
  maxdist = 0;
  data.altminimum = 100000;
  data.tsprintstart = 0;
  istart = 0;
  data.waypointbearing = 0;
  loc_proj.Latitude = 0;
  loc_proj.Longitude = 0;

  data.solution_FAI_triangle.valid = false;
  data.solution_FAI_sprint.valid = false;
  data.solution_FAI_classic.valid = false;

  data.solution_FAI_triangle.finished = false;
  data.solution_FAI_sprint.finished = false;
  data.solution_FAI_classic.finished = false;

}


void OLCOptimizer::Clear() {
  stop = false;
}


#define sindex(y,x) (indexval[y]+x)

/*
int OLCOptimizer::sindex(int y, int x) {
  bool err = true;
  if (x<y) {
    err = true;
  }
  int r = indexval[y]+x;
  if (r>=MATSIZE) {
    err = true;
  }
  return r;
}
*/

int OLCOptimizer::initdmval() {
  double d_fak = CONST_D_FAK; /* FAI-Erdradius in metern */

  /* F?r schnellere Berechnung sin/cos-Werte merken */
  double sinlat[MAX_OLC_POINTS];
  double coslat[MAX_OLC_POINTS];
  double lonrad[MAX_OLC_POINTS];

  double latrad, sli, cli, lri;
  int i, j, dist, cmp;

  i=0;
  for (j=0; j<pnts; j++) {
    indexval[j]= i-j;
    i+= (pnts-j);
  }

  for(i=pnts-1;i>=0;i--) {
    /* alle Punkte ins Bogenma?
       umrechnen und sin/cos Speichern */
    lonrad[i] = data.locpnts[i].Longitude * DEG_TO_RAD;
    latrad = data.locpnts[i].Latitude * DEG_TO_RAD;
    sinlat[i] = sin(latrad);
    coslat[i] = cos(latrad);
    dmval[sindex(i,i)] = 0;
    /* Diagonale der Matrix mit Distanz 0 f?llen */
  }

  // System.out.println("initializing distances..\n");
  maxdist = 0; /* maximale Distanz zwischen zwei Punkten neu
		  berechnen */

  cmp = pnts-1;
  /* Schleifenvergleichswert f?r schnelle Berechnung vorher merken */

  // JMW: this calculation is very slow!  N^2 cos!
  for(i=0;i<cmp;i++) { /* diese Schleife NICHT R?CKW?RTS!!! */
    sli = sinlat[i]; cli = coslat[i]; lri = lonrad[i];
    for(j=i+1;j<pnts;j++) {
      dist = (int)(d_fak*acos(sli*sinlat[j]
			      + cli*coslat[j]*cos(lri-lonrad[j])
			      )+0.5);  /* auf meter runden */
      dmval[sindex(i,j)]= (unsigned short)(dist/DISTANCEUNITS);
      maxdist = max(dist,maxdist);
      /* ggf. weiteste Distanz merken */
    }
  }

  // "maximal distance between 2 points: " + maxdist + " meters\n");
  return 0;
}


// find point k between i and j to give greatest distance ik and kj
// k = isplit(i,j)
int OLCOptimizer::initisplit() {
  int maxab, i, j, k, d, ibest;

  for(i=0;i<pnts-1;i++) {

    isplit[sindex(i,i)]= (unsigned short)i;

    for(j=i+1; j<pnts;j++) {

      maxab = 0;
      ibest = j;

      for(k=i+1; k<j; k++) {
	d = dmval[sindex(i,k)] + dmval[sindex(k,j)];
	if (d>maxab) {
	  maxab = d;
	  ibest = k;
	}
      }
      isplit[sindex(i,j)]= (unsigned short)ibest;
    }
  }
  return 0;
}


int OLCOptimizer::initistartsprint() {

  int i, j, altend, tend, ibest, alt, t, altmin;
  for (i=pnts-1; i>=0; i--) {
    // end point
    altend = data.altpntslow[i];
    tend = data.timepnts[i];
    ibest = i;
    altmin = altend;

    for (j=1; j<i; j++) {
      // start point... we need latest lowest within 9000 seconds
      alt = data.altpntslow[j];
      t = data.timepnts[j];
      if ((alt<=altmin)&&(tend-t<9000)) {
	ibest = j;
	altmin = alt;
      }
    }
    istartsprint[i]= (unsigned short)max(1,ibest);
  }
  return 0;
}


/*
  ibestend[sindex(i,j)] is index of furthest end point from j which is valid
                        from start height i
*/
int OLCOptimizer::initibestend() {

  int i,j,k, maxclassic, dh, ibestclassic, d;

  for(i=0;i<pnts;i++) { // for all start points
    for(j=i;j<pnts; j++) { // for all second last points

      maxclassic = 0;
      ibestclassic = i;

      for(k=j+1; k<pnts; k++) { // for all last points

	dh = data.altpntshigh[k]-data.altpntslow[i];
	d = dmval[sindex(j,k)];

	if (dh>= -1000) {
	  if (d>=maxclassic) {
	    maxclassic = d;
	    ibestclassic = k;
	  }
	}
      }
      ibestendclassic[sindex(i,j)]= (unsigned short)ibestclassic;
    }
  }
  return 0;
}


void OLCOptimizer::thin_data() {
  // reduce number of waypoints until smaller than buffer

  if (data.pnts_in<MAX_OLC_POINTS) {
    return;
  }

  Lock();

  int i;
  int nistart = 5;
  for (i=0; i<data.pnts_in; i++) {
    if (data.timepnts[i]>= data.tsprintstart) {
      nistart = i;
      break;
    }
  }

  nistart = 5; // JMW check this!
  double contractfactor = 0.8;
  int num_to_prune = (int)((1.0-contractfactor)*MAX_OLC_POINTS);
  int num_pruned = 0;
  bool prune_from_end = true;

  while (num_pruned < num_to_prune) {
    // don't erase last point and don't erase up to start
    if (prune_from_end) {
      prune_from_end = false; // on second pass (if necessary) prune from start
      for (i= data.pnts_in-3; i>nistart+1; i--) {
        // prune from end since at current distance threshold we won't have
        // pruned later ones.
        //
        // by the time the buffer fills at the current distance threshold,
        // pruning will have occurred or been checked on all points
        // going back to the start,
        double d;
        DistanceBearing(data.locpnts[i], data.locpnts[i-1],
                        &d, NULL);
        if (d<data.distancethreshold) {
          data.timepnts[i] = -1; // mark it for deletion
          i--;                   // but not the next one
          num_pruned++;
        }
      }
    } else {
      for (i= nistart+2; i<data.pnts_in-4; i++) {
        // prune from start on second pass since this takes decrease
        // in resolution from start of flight;
        // what's more, it will prune all the points in the buffer
        double d;
        DistanceBearing(data.locpnts[i], 
                        data.locpnts[i-1],
                        &d, NULL);
        if (d<data.distancethreshold) {
          data.timepnts[i] = -1; // mark it for deletion
          i++;                   // but not the next one
          num_pruned++;
        }
      }
    }

    // now shuffle points along
    int j;
    i = nistart+1;
    j = i;
    int pnts_in_new=0;
    int altlowmerge=100000;
    while (j< data.pnts_in) {
      if (data.timepnts[j]!= -1) {
	data.locpnts[i] = data.locpnts[j];
	data.timepnts[i] = data.timepnts[j];
	data.altpntshigh[i] = data.altpntshigh[j];
	data.altpntslow[i] = min(data.altpntslow[j],altlowmerge);
	altlowmerge = data.altpntslow[j];
	i++;
	pnts_in_new = i;
      } else {
	altlowmerge = min(data.altpntslow[j], altlowmerge);
      }
      j++;
    }

    data.pnts_in = pnts_in_new;
    if (num_pruned < num_to_prune) {
      // can't prune any more at this distance threshold, so need to increase it
      data.distancethreshold /= contractfactor;
    }
  }
  if (data.pnts_in>=MAX_OLC_POINTS) {
    // error!
    data.pnts_in = MAX_OLC_POINTS-1;
  }
  Unlock();
}


bool OLCOptimizer::addPoint(const GEOPOINT &loc,
                            double alt,
                            double bearing,
			    double time,
			    const SETTINGS_COMPUTER &settings) {
  static GEOPOINT loclast;
  static int alt1 = 0;
  static int alt2 = 0;

  ScopeLock protect(mutexOLC);

  if (data.pnts_in==0) {
    loclast = loc;
    alt1 = 0;
    alt2 = 0;
  }

  if (busy) return false; // don't add data while in analysis

  Lock();

  data.waypointbearing = bearing;

  int ialt = (int)alt;
  int i;

  bool isminimum = false;
  if ((ialt>alt1) && (alt2>alt1)) {
    isminimum = true;
  }
  switch(settings.OLCRules) {
  case 0: // sprint
    isminimum &= (ialt<data.altminimum);
    break;
  case 1: // classic
    isminimum &= (ialt<data.altminimum-1000);
    break;
  case 2: // classic
    isminimum &= (ialt<data.altminimum-1000);
    break;
  }
  if (isminimum && settings.EnableOLC) {
    data.altminimum = min(ialt,data.altminimum);
    data.tsprintstart = (long)time;
  }

  alt2= alt1;
  alt1= ialt;

  double tmpd;
  DistanceBearing(loc, loclast, &tmpd, NULL);
  if ((tmpd>data.distancethreshold)
      || (data.pnts_in==0) || (isminimum)) {

    loclast = loc;

    i = data.pnts_in;

    data.timepnts[i] = (long)time;
    data.locpnts[i] = loc;
    data.altpntslow[i] = ialt;
    data.altpntshigh[i] = ialt;

    if (data.pnts_in<MAX_OLC_POINTS) {
      data.pnts_in++;
    }
    thin_data();
  } else {
    if (data.pnts_in>0) {
      i = data.pnts_in-1;
      if (ialt<data.altpntslow[i]) {
	// if new low at this location, replace the altitude and time
	// this gets better accuracy for start location
	data.timepnts[i] = (long)time;
	data.altpntslow[i] = ialt;
      } else if (ialt>data.altpntshigh[i]) {
	data.timepnts[i] = (long)time;
	data.altpntshigh[i] = ialt;
      }
    }
  }

  Unlock();

  return isminimum;
  // detect new start and return true if start detected
  // maybe make start above safety arrival height?
  //
  // also detect task finish here?
}


int OLCOptimizer::getN() {
  if (busy) return 0; // Inhibit display if busy optimising
  return pnts;
}

double OLCOptimizer::getAltitudeHigh(int i) {
  return data.altpntshigh[i];
}

double OLCOptimizer::getTime(int i) {
  return data.timepnts[i];
}

const GEOPOINT& OLCOptimizer::getLocation(int i) {
  return data.locpnts[i];
}


void OLCOptimizer::SetLine() {

  Lock();
  pnts = data.pnts_in; // save value in case we get new data while
                       // performing the analysis/display
  Unlock();

}


bool OLCOptimizer::Optimize(const SETTINGS_COMPUTER &settings, bool isflying) {
  SetLine();

  flying = isflying;

#ifdef DEBUG_OLC
  DWORD tm =GetTickCount();
#endif

  bool retval = (optimize_internal(settings) == 0);
  Clear();

  if (retval) {
#ifdef DEBUG_OLC
    DebugStore("%d %d # OLC\n", pnts, GetTickCount()-tm);
#endif
  }

  return retval;
}


void OLCOptimizer::UpdateSolution(int dbest, int tbest,
				  int p1, int p2, int p3, int p4, int p5,
				  int p6, int p7,
				  OLCSolution* solution, double score, bool finished) {
  bool improved = !(solution->valid);
  if (score>0) {
    if ((score> solution->score)
	||(!finished)
	||(finished && (!solution->finished))) {
      improved = true;
    }
    if (improved) {
      solution->valid = true;
      solution->finished = finished;
      solution->distance = dbest*DISTANCEUNITS;
      solution->score = score;
      solution->location[0] = data.locpnts[(p1)];
      solution->location[1] = data.locpnts[(p2)];
      solution->location[2] = data.locpnts[(p3)];
      solution->location[3] = data.locpnts[(p4)];
      solution->location[4] = data.locpnts[(p5)];
      solution->location[5] = data.locpnts[(p6)];
      solution->location[6] = data.locpnts[(p7)];
      solution->time = tbest;
    }
  }
}


int OLCOptimizer::optimize_internal(const SETTINGS_COMPUTER &settings) {

  busy = true;

  if (pnts<5) {
    // only <5 points given, no optimization
    busy = false;
    return 0;
  }

  if( initdmval() != 0 ) { busy=false; return 1; }
  if( initisplit() != 0 ) { busy=false; return 1; }
  if( initistartsprint() != 0 ) { busy=false; return 1; }
  if( initibestend() != 0 ) { busy=false; return 1; }

  int i1;
  istart = 0;
  for (i1=0; i1<pnts-5; i1++) {
    if (data.timepnts[i1]>= data.tsprintstart) {
      istart = i1;
      break;
    }
  }

  // World league (sprint):
  //   start height <= finish height
  //   2.5 hours max
  //   start+3turnpoints+finish
  //
  // FAI triangle
  //   start height <= finish height+1000m
  //
  // Classic:
  //   start height <= finish height+1000m
  //   start+5turnpoints+finish
  //   no time limit
  //   points for second last leg 0.8
  //   points for last leg 0.6

  switch(settings.OLCRules) {
  case 0:
    scan_sprint(settings);
    break;
  case 1:
    scan_triangle(settings);
    break;
  case 2:
    scan_classic(settings);
    break;
  }

  busy = false;
  return 0;
}



/*
  TODO accuracy: OLC handicap, estimated score

  Rotary buffer, and save previous best when doing optimisation.
  This means we only need a buffer large enough for 2.5 hours,
  so we can plan on update every one minute seconds for 200 samples max
  (allowing small margin).
  This gives optimisation time of 2 seconds.
  At 100 knots, one minute = 3.3 km.  Set distance threshold to 1 km.

*/


int OLCOptimizer::triangle_legal(int i1, int i2, int i3, int i4) {
  int a,b,c,d;
  int minleg, maxleg;
  int Dist;
  if (i3<=i4)
    c = dmval[sindex(i3,i4)];
  else
    c = dmval[sindex(i4,i3)];
  if (i1<=i4)
    d = dmval[sindex(i1,i4)];
  else
    d = dmval[sindex(i4,i1)];
  if (5*d>c) {
    return 0; // i4 must be within 20% of c distance to i1
  }
  a = dmval[sindex(i1,i2)];
  b = dmval[sindex(i2,i3)];
  Dist = a+b+c-d;
  if (Dist<500000/DISTANCEUNITS) {
    minleg = min(a,min(b,c));
    // <500km, 28% min
    if (minleg*25>=Dist*7) {
      return Dist;
    } else {
      return 0;
    }
  } else {
    // >500km, 25% min 45% max
    minleg = min(a,min(b,c));
    maxleg = max(a,max(b,c));
    if ((minleg*4>=Dist)&&(maxleg*20<=9*Dist)) {
      return Dist;
    } else {
      return 0;
    }
  }
}


int OLCOptimizer::scan_triangle(const SETTINGS_COMPUTER &settings) {
  int i2, i3, i4, i5;
  int dh, d;

  int bestdist = 0;
  int i2best=0, i3best=0, i4best=0, i5best=0;

  //  i1 is ignored

  // O(N^3)

  int dfurther=0;
  bool finished=false;
  bool canfinish=false;
  int ttogo=0;
  int ttogobest=0;

  for (i2=1; i2<pnts-3; i2++) {
    for (i5=pnts-1; i5>i2+2; i5--) {

      int dtogo = dmval[sindex(i2,i5)];

      // FAI triangle
      //   start height <= finish height+1000m
      dh = data.altpntshigh[i5]+1000-data.altpntslow[i2];
      if (dh<0) continue;

      canfinish = false;
      ttogo = 0;
      if (dh>0) {
	dfurther = iround(GlidePolar::bestld*dh/DISTANCEUNITS);
	if (dfurther>dtogo) {
	  canfinish = true;
	  ttogo = iround(dtogo*DISTANCEUNITS/GlidePolar::Vbestld);
	}
      } else {
	dfurther = 0;
      }

      for (i4=i2+2; i4<i5; i4++) {
	i3 = isplit[sindex(i2,i4)];

	// check optimal triangle given i4, i3, assuming finished
	d = triangle_legal(i2,i3,i4,i5);
	if (d>bestdist) {
	  i2best = i2;
	  i3best = i3;
	  i4best = i4;
	  i5best = i5;
	  bestdist = d;
	  finished = true;
	}
	if (canfinish && flying) {
	  d = triangle_legal(i2,i3,i4,i2)-dtogo;
	  if ((d>bestdist)&&(dmval[sindex(i2,i4)]>5*dtogo)) {
	    i2best = i2;
	    i3best = i3;
	    i4best = i4;
	    i5best = i2;
	    bestdist = d;
	    finished = false;
	    ttogobest = ttogo;
	  }
	}
      }
    }
  }

  if (bestdist>0) {
    double score = bestdist*100/(settings.Handicap)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]-data.timepnts[(i2best)];
    if (!finished) {
      loc_proj = data.locpnts[i2];
      t += ttogobest;
    }
    UpdateSolution(bestdist, t, i2best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_triangle, score, finished);
    return 0;
  } else {
    return 1;
  }

}



int OLCOptimizer::scan_sprint_finished(const SETTINGS_COMPUTER &settings) {
  int i1,i2,i3,i4,i5, d, bestdist;
  int i1best=0, i2best=0, i3best=0, i4best=0, i5best=0;

  // O(N^2)

//  bool taskfinished=false;
  bestdist = 0;

  // detect task end
  for (i5=pnts-1; i5>5; i5--) {
    i1 = istartsprint[i5];
    if (data.altpntslow[i5]>= data.altpntslow[i1]) {
      if (data.timepnts[i5]-data.timepnts[i1-1]>=9000) {

	for (i3=i1+2; i3<i5-1; i3++) {
	  i2 = isplit[sindex(i1,i3)];
	  i4 = isplit[sindex(i3,i5)];

	  d = dmval[sindex(i1,i2)]
	    +dmval[sindex(i2,i3)]
	    +dmval[sindex(i3,i4)]
	    +dmval[sindex(i4,i5)];

	  if (d>bestdist) {
	    bestdist = d;
	    i1best = i1;
	    i2best = i2;
	    i3best = i3;
	    i4best = i4;
	    i5best = i5;
	  }
	}
      }
    }
  }

  if (bestdist>0) {
    double score = bestdist*100/(settings.Handicap*2.5)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]-data.timepnts[(i1best)];
    UpdateSolution(bestdist, t, i1best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_sprint, score, true);
    return 0;
  } else {
    return 1;
  }
}


int OLCOptimizer::scan_sprint(const SETTINGS_COMPUTER &settings) {
  int retval=0;
  // first scan for a finished sprint
  // then see if improvement can be made with final glide at excess altitude

  retval = scan_sprint_finished(settings);
  if (flying) {
    retval |= scan_sprint_inprogress(settings);
  }
  return retval;
}


int OLCOptimizer::scan_sprint_inprogress(const SETTINGS_COMPUTER &settings) {
  int i1,i2,i3,i4,i5, d, bestdist;
  int i1best=0, i2best=0, i3best=0, i4best=0, i5best=0;

  bestdist = 0;

  // now calculate estimate:
  // assume flying from last point to active waypoint
  // we already know time remaining to waypoint.
  // assume above final glide, and in final glide mode
  // may not be flying Mc speed, because may need to rush to
  // expend height in given remaining time.
  // but remaining time is unknown!

  // what we can do is search for best distance across height loss from current
  // assuming height loss will be taken in maximum time remaining

  // build a table giving for a start i and height at final point to burn,
  // time remaining (to burn the height)
  //

  // O(N^2)

  i5 = pnts-1;
  i1 = istart;

  GEOPOINT locend = data.locpnts[(i5)];
  int dh = data.altpntslow[(i5)]-data.altpntslow[(i1)];
  int dt = 9000-(data.timepnts[(i5)]-data.timepnts[(i1)]);

  if (!flying) {
    return 1; // finished!
  }

  if (dh<0) {
    return 1; // finished! (altitude reached)
  }
  if (dt<0) {
    return 1; // finished! (over time)
  }

  //
  // if this data doesn't change, e.g. don't go lower than start altitude,
  // then it is fast and can be performed online

  double sinkrate = ((double)dh)/dt;
  // we have dh of height to burn off in dt seconds
  // this is a vertical velocity, which gives us
  // a target speed

  double Vopt;
  int dfurther;

  // TODO accuracy: Adjust for wind!
  if (sinkrate>=GlidePolar::minsink) {
    // no need to climb to make it
    // (TODO accuracy: later work out time adjustment for climb here, but for
    // now we just assume we won't be climbing again)

    Vopt = GlidePolar::FindSpeedForSinkRate(sinkrate);
  } else {
    // can't make it without further climb.  For now, just calculate
    // what the best you can do with remaining height.

    Vopt = GlidePolar::Vbestld;
    sinkrate = (Vopt/GlidePolar::bestld);
    dt = iround(dh/sinkrate);
  }
  dfurther = (int)(Vopt*dt/DISTANCEUNITS); // neglects wind speed!  we can correct this
  // TODO accuracy: OLC further

  // we can calculate the optimal instantaneous
  // MC value for this given the netto velocity
  // So auto mc needs to find highest Mc that results in dh=0 and largest t_ETE<9000.

  // only i4 will affect optimum direction, so this should be outer loop

  for (i4=i5-1; i4>i1+2; i4--) {

    int d0 = dmval[sindex(i4,i5)];
    // dfurther is independent of i3 and i4 so it won't affect optimum
    // if direction is free

    // Data.Waypointbearing
    // note this outer loop is linear in N

    d0 += dfurther; // big assumption, ignores effect of wind and waypoint bearing
    // but this is reasonable as it assumes pilot is making bearing decisions
    // independent of track so far.

    for (i3=i1+2; i3<i4; i3++) {
      i2 = isplit[sindex(i1,i3)];

      d = dmval[sindex(i1,i2)]
	+dmval[sindex(i2,i3)]
	+dmval[sindex(i3,i4)]+d0;

      if (d>bestdist) {
	bestdist = d;
	i1best = i1;
	i2best = i2;
	i3best = i3;
	i4best = i4;
	i5best = i5;
	// TODO accuracy: find equivalent Mc value?, Command Vopt?
      }
    }
  }

  if (bestdist>0) {

    FindLatitudeLongitude(locend,
                          data.waypointbearing,
                          dfurther*DISTANCEUNITS,
                          &loc_proj);

    double score = bestdist*100/(settings.Handicap*2.5)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]+dt-data.timepnts[(i1best)];
    UpdateSolution(bestdist, t, i1best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_sprint, score, false);
    return 0;
  } else {
    return 1;
  }
}


int OLCOptimizer::scan_classic(const SETTINGS_COMPUTER &settings) {
  int i1,i2,i3,i4,i5,i6,i7, d, bestdist, dh;
  int i1best=0, i2best=0, i3best=0, i4best=0, i5best=0, i6best=0, i7best=0;

  bestdist = 0;
  bool finished = false;

  //  for (i1=0; i1<pnts-6; i1++) {
  i1= istart; // force start to first point, to keep this algorithm  O(N^3)

  // there is no time penalty so earliest valid start is always best?
  int dfurther = 0;
  int dfurtherbest = 0;

  // O(N^3)

  for (i6=i1+5; i6<pnts; i6++) {

    // TODO code: eliminate this or reduce it to search across i6,
    // since i1 is given
    i7 = ibestendclassic[sindex(i1,i6)];

    if (i7>=i6) { // valid end point found

      for (i3=i1+2; i3<i6-3; i3++) {
	i2 = isplit[sindex(i1,i3)];

	for (i5=i3+2; i5<=i6; i5++) {
	  i4 = isplit[sindex(i3,i5)];

	  dfurther = 0;
	  if (i7==pnts-1) {
	    // check if can travel further with final glide
	    dh = data.altpntslow[i7]-data.altpntslow[i1];
	    if (dh>0) {
	      dfurther = (int)(GlidePolar::bestld*dh/DISTANCEUNITS);
	    }
	  }

	  d = (dmval[sindex(i1,i2)]*5
	       +dmval[sindex(i2,i3)]*5
	       +dmval[sindex(i3,i4)]*5
	       +dmval[sindex(i4,i5)]*5
	       +dmval[sindex(i5,i6)]*4
	       +dmval[sindex(i6,i7)]*3)/5+dfurther;

	  if (d>bestdist) {
	    bestdist = d;
	    i1best = i1;
	    i2best = i2;
	    i3best = i3;
	    i4best = i4;
	    i5best = i5;
	    i6best = i6;
	    i7best = i7;
	    if ((dfurther==0)||(!flying)) {
	      finished = true;
	    } else {
	      dfurtherbest = dfurther;
	      finished = false;
	    }
	  }
	}
      }
    }
  }

  if (bestdist>0) {
    double score = bestdist*100/settings.Handicap/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i7best)]-data.timepnts[(i1best)];
    if (!finished) {
      t += (int)(dfurtherbest*DISTANCEUNITS/GlidePolar::Vbestld);

      FindLatitudeLongitude(data.locpnts[i7best],
                            data.waypointbearing,
                            dfurther*DISTANCEUNITS,
                            &loc_proj);
    }
    UpdateSolution(bestdist, t, i1best, i2best, i3best, i4best,
                   i5best, i6best, i7best,
		   &data.solution_FAI_classic, score, finished);
    return 0;
  } else {
    return 1;
  }
}



// Tip: define AAT tasks for OLC, so you can use the AAT remaining time,
// range buttons etc to adjust

// Use of ArmAdvance to reset starts?
// Symbol on screen "ADV+" if valid for arm


double OLCOptimizer::getDt(const SETTINGS_COMPUTER &settings) {
  switch(settings.OLCRules) {
  case 0:
    return data.solution_FAI_sprint.time;
    break;
  case 1:
    return data.solution_FAI_triangle.time;
    break;
  case 2:
    return data.solution_FAI_classic.time;
    break;
  default:
    return 0.0;
  }
}

double OLCOptimizer::getD(const SETTINGS_COMPUTER &settings) {
  switch(settings.OLCRules) {
  case 0:
    return data.solution_FAI_sprint.distance;
    break;
  case 1:
    return data.solution_FAI_triangle.distance;
    break;
  case 2:
    return data.solution_FAI_classic.distance;
    break;
  default:
    return 0;
  }
}

double OLCOptimizer::getValid(const SETTINGS_COMPUTER &settings) {
  switch(settings.OLCRules) {
  case 0:
    return data.solution_FAI_sprint.valid;
    break;
  case 1:
    return data.solution_FAI_triangle.valid;
    break;
  case 2:
    return data.solution_FAI_classic.valid;
    break;
  default:
    return false;
  }
}

double OLCOptimizer::getScore(const SETTINGS_COMPUTER &settings) {
  switch(settings.OLCRules) {
  case 0:
    return data.solution_FAI_sprint.score;
    break;
  case 1:
    return data.solution_FAI_triangle.score;
    break;
  case 2:
    return data.solution_FAI_classic.score;
    break;
  default:
    return 0;
  }
}

double OLCOptimizer::getFinished(const SETTINGS_COMPUTER &settings) {
  switch(settings.OLCRules) {
  case 0:
    return data.solution_FAI_sprint.finished;
    break;
  case 1:
    return data.solution_FAI_triangle.finished;
    break;
  case 2:
    return data.solution_FAI_classic.finished;
    break;
  default:
    return false;
  }
}
