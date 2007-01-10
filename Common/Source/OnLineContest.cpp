#include "stdafx.h"
#include "XCSoar.h"
#include "OnLineContest.h"
#include "Utils.h"
#include "McReady.h"
#include <math.h>


#define CONST_D_FAK 6371000.0
#define DISTANCETHRESHOLD 1000
#define DISTANCEUNITS 100

bool EnableOLC = false;
int OLCRules = 0;
// 0: sprint task
// 1: FAI triangle
// 2: OLC classic
int Handicap = 108; // LS-3


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

  lat_proj = 0;
  lon_proj = 0;

  flying = true;

  Clear();
  ResetFlight();
}

OLCOptimizer::~OLCOptimizer() {
  Clear();
  ResetFlight();
};


void OLCOptimizer::ResetFlight() {
  data.pnts_in = 0;
  pnts = 0;
  data.distancethreshold = DISTANCETHRESHOLD/2.0; // 500 meters
  maxdist = 0;
  data.altminimum = 100000;
  data.tsprintstart = 0;
  istart = 0;
  data.waypointbearing = 0;
  lat_proj = 0;
  lon_proj = 0;

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
    lonrad[i] = data.lonpnts[i] * DEG_TO_RAD;
    latrad = data.latpnts[i] * DEG_TO_RAD;
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

  int i;
  int nistart = 5;
  for (i=0; i<data.pnts_in; i++) {
    if (data.timepnts[i]>= data.tsprintstart) {
      nistart = i;
      break;
    }
  }

  nistart = 5;
  double contractfactor = 0.8;

  while (data.pnts_in> MAX_OLC_POINTS*contractfactor) {
    data.distancethreshold /= contractfactor;
    // don't erase last point and don't erase up to start
    for (i= data.pnts_in-3; i>nistart+1; i--) {
      double d;
      DistanceBearing(data.latpnts[i], data.lonpnts[i],
                      data.latpnts[i-1], data.lonpnts[i-1],
                      &d, NULL);
      if (d<data.distancethreshold) {
	data.timepnts[i] = -1; // mark it for deletion
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
	data.latpnts[i] = data.latpnts[j];
	data.lonpnts[i] = data.lonpnts[j];
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
  }
  if (data.pnts_in>=MAX_OLC_POINTS) {
    // error!
    data.pnts_in = MAX_OLC_POINTS-1;
  }
}


bool OLCOptimizer::addPoint(double lon, double lat, double alt,
                            double bearing,
			    double time) {
  static double lonlast;
  static double latlast;
  static int alt1 = 0;
  static int alt2 = 0;

  if (data.pnts_in==0) {
    latlast = lat;
    lonlast = lon;
    alt1 = 0;
    alt2 = 0;
  }

  if (busy) return false; // don't add data while in analysis

  data.waypointbearing = bearing;

  int ialt = (int)alt;
  int i;

  bool isminimum = false;
  if ((ialt>alt1) && (alt2>alt1)) {
    isminimum = true;
  }
  switch(OLCRules) {
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
  if (isminimum) {
    data.altminimum = min(ialt,data.altminimum);
    data.tsprintstart = (long)time;
  }

  alt2= alt1;
  alt1= ialt;

  double tmpd;
  DistanceBearing(lat, lon, latlast, lonlast, &tmpd, NULL);
  if ((tmpd>data.distancethreshold)
      || (data.pnts_in==0) || (isminimum)) {

    latlast = lat;
    lonlast = lon;

    i = data.pnts_in;

    data.timepnts[i] = (long)time;
    data.latpnts[i] = lat;
    data.lonpnts[i] = lon;
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

  return isminimum;
  // detect new start and return true if start detected
  // maybe make start above safety arrival height?
  //
  // also detect task finish here?
};


int OLCOptimizer::getN() {
  return pnts;
}

double OLCOptimizer::getLatitude(int i) {
  return data.latpnts[i];
}

double OLCOptimizer::getLongitude(int i) {
  return data.lonpnts[i];
}


void OLCOptimizer::SetLine() {

  LockFlightData();
  pnts = data.pnts_in; // save value in case we get new data while
		  // performing the analysis/display
  UnlockFlightData();

}


bool OLCOptimizer::Optimize(bool isflying) {
  SetLine();

  flying = isflying;

  DWORD tm =GetTickCount();

  bool retval = (optimize_internal() == 0);
  Clear();

  if (retval) {
#ifdef DEBUG
    char tmptext[100];
    sprintf(tmptext,"%d %d # OLC\n", pnts, GetTickCount()-tm);
    DebugStore(tmptext);
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
      solution->latitude[0] = data.latpnts[(p1)];
      solution->longitude[0] = data.lonpnts[(p1)];
      solution->latitude[1] = data.latpnts[(p2)];
      solution->longitude[1] = data.lonpnts[(p2)];
      solution->latitude[2] = data.latpnts[(p3)];
      solution->longitude[2] = data.lonpnts[(p3)];
      solution->latitude[3] = data.latpnts[(p4)];
      solution->longitude[3] = data.lonpnts[(p4)];
      solution->latitude[4] = data.latpnts[(p5)];
      solution->longitude[4] = data.lonpnts[(p5)];
      solution->latitude[5] = data.latpnts[(p6)];
      solution->longitude[5] = data.lonpnts[(p6)];
      solution->latitude[6] = data.latpnts[(p7)];
      solution->longitude[6] = data.lonpnts[(p7)];
      solution->time = tbest;
    }
  }
}


int OLCOptimizer::optimize_internal() {

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

  switch(OLCRules) {
  case 0:
    scan_sprint();
    break;
  case 1:
    scan_triangle();
    break;
  case 2:
    scan_classic();
    break;
  }

  busy = false;
  return 0;
}



/*
  TODO:

  handicap in glide polar?

  Estimate score?

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


int OLCOptimizer::scan_triangle() {
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
    double score = bestdist*100/(Handicap)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]-data.timepnts[(i2best)];
    if (!finished) {
      lat_proj = data.latpnts[i2];
      lon_proj = data.lonpnts[i2];
      t += ttogobest;
    }
    UpdateSolution(bestdist, t, i2best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_triangle, score, finished);
    return 0;
  } else {
    return 1;
  }

}



int OLCOptimizer::scan_sprint_finished() {
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
    double score = bestdist*100/(Handicap*2.5)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]-data.timepnts[(i1best)];
    UpdateSolution(bestdist, t, i1best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_sprint, score, true);
    return 0;
  } else {
    return 1;
  }
}


int OLCOptimizer::scan_sprint() {
  int retval=0;
  // first scan for a finished sprint
  // then see if improvement can be made with final glide at excess altitude

  retval = scan_sprint_finished();
  if (flying) {
    retval |= scan_sprint_inprogress();
  }
  return retval;
}


int OLCOptimizer::scan_sprint_inprogress() {
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

  i5 = pnts-1;
  i1 = istart;

  double latend = data.latpnts[(i5)];
  double lonend = data.lonpnts[(i5)];
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

  // TODO: Adjust for wind!
  if (sinkrate>=GlidePolar::minsink) {
    // no need to climb to make it
    // (TODO: later work out time adjustment for climb here, but for
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

    for (i3=i1+2; i3<i4; i3++) {  // O(N^2)
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
	// TODO: find equivalent Mc value?
	// Command Vopt?
      }
    }
  }

  if (bestdist>0) {

    FindLatitudeLongitude(latend,
                          lonend,
                          data.waypointbearing,
                          dfurther*DISTANCEUNITS,
                          &lat_proj,
                          &lon_proj);

    double score = bestdist*100/(Handicap*2.5)/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i5best)]+dt-data.timepnts[(i1best)];
    UpdateSolution(bestdist, t, i1best, i2best, i3best, i4best, i5best, i5best, i5best,
		   &data.solution_FAI_sprint, score, false);
    return 0;
  } else {
    return 1;
  }
}


int OLCOptimizer::scan_classic() {
  int i1,i2,i3,i4,i5,i6,i7, d, bestdist, dh;
  int i1best=0, i2best=0, i3best=0, i4best=0, i5best=0, i6best=0, i7best=0;

  bestdist = 0;
  bool finished = false;

  //  for (i1=0; i1<pnts-6; i1++) {
  i1= istart; // force start to first point, to keep this algorithm  O(N^3)

  // there is no time penalty so earliest valid start is always best?
  int dfurther = 0;
  int dfurtherbest = 0;

  for (i6=i1+5; i6<pnts; i6++) {

    // TODO: eliminate this or reduce it to search across i6, since i1 is given
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
    double score = bestdist*100/Handicap/(1000.0/DISTANCEUNITS);
    int t = data.timepnts[(i7best)]-data.timepnts[(i1best)];
    if (!finished) {
      t += (int)(dfurtherbest*DISTANCEUNITS/GlidePolar::Vbestld);

      FindLatitudeLongitude(data.latpnts[i7best],
                            data.lonpnts[i7best],
                            data.waypointbearing,
                            dfurther*DISTANCEUNITS,
                            &lat_proj, &lon_proj);
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
