#include "stdafx.h"
#include "XCSoar.h"
#include "OnLineContest.h"
#include "Utils.h"
#include <math.h>


#define MAXPOINTS 300
#define CONST_D_FAK 6371000.0
#define DISTANCETHRESHOLD 1.0

int OLCRules = 0;
// 0: sprint task
// 1: FAI triangle


OLCOptimizer::OLCOptimizer() {

  pnts = 0;
  pnts_in = 0;
  pnts_offset = 0;
  pnts_offset_in = 0;

  dmval = NULL;
  dmin = NULL;
  dminj = NULL;
  dmini = NULL;
  maxenddist = NULL;
  maxendpunkt = NULL;

  lat_proj = 0;
  lon_proj = 0;
  alt_proj = 0;
  project = false;

  altpnts = (int*)malloc(sizeof(int)*MAXPOINTS);
  latpnts = (double*)malloc(sizeof(double)*MAXPOINTS);
  lonpnts = (double*)malloc(sizeof(double)*MAXPOINTS);
  timepnts = (long*)malloc(sizeof(long)*MAXPOINTS);

  Clear();
  ResetFlight();
}

OLCOptimizer::~OLCOptimizer() {
  Clear();
  if (altpnts) { free(altpnts); altpnts=NULL;}
  if (latpnts) { free(latpnts); latpnts=NULL;}
  if (lonpnts) { free(lonpnts); lonpnts=NULL;}
  if (timepnts) { free(timepnts); timepnts=NULL;}
  pnts = 0;
  ResetFlight();
};


void OLCOptimizer::ResetFlight() {
  pnts_in = 0;
  pnts_offset_in = 0;

  maxdist = 0;
  max1 = 0;
  max2 = 0;
  max3 = 0;
  max4 = 0;
  max5 = 0;
  max1fai=0; max2fai=0; max3fai=0; max4fai=0; max5fai=0;
  max1flach=0; max2flach=0; max3flach=0; max4flach=0; max5flach=0;
  maxroute=0; bestfai=0; bestflach=0;

  solution_FAI_triangle.valid = false;
  solution_FAI_sprint.valid = false;

}

void OLCOptimizer::Clear() {
  stop = false;

  if (dmval) { free (dmval); dmval=NULL;}
  if (dmin) { free (dmin); dmin=NULL;}
  if (dminj) { free(dminj); dminj=NULL;}
  if (dmini) { free(dmini); dmini=NULL;}
  if (maxenddist) { free(maxenddist); maxenddist=NULL;}
  if (maxendpunkt) { free(maxendpunkt); maxendpunkt=NULL;}
}


#define rotindex(x) ((x+pnts_offset)%MAXPOINTS)
#define rotindexin(x) ((x+pnts_offset_in)%MAXPOINTS)

    /*
     * Matrix mit den kleinsten Abst?nden zwischen Start- und Endpunkt
     * f?r gegebenen ersten und dritten Wendepunkt.
     * Beispiel: dmin[5][10] ist die kleinstm?gliche Distanz d zwischen
     * Start- und Endpunkt, wenn Punkt 5 der erste und Punkt 10 der 3te
     * Wendepunkt ist. dmini[5][10] leifert dann, den Index des zugeh?rigen
     * Startpunktes und dminj[5][10] den Index des Endpunktes f?r diese
     * minimale Distanz
     * Dies Matritzen werden mit quadratischem Aufwand vorab berechnet,
     * da sie zur Bestimmung von Dreiecken bei der Optimierung in der
     * innersten Schleife f?r die 3 Wendepunkte n^3 mal immer wieder
     * ben?tigt werden. Dadurch kann der Gesamtrechaufwand in der Gr??enordnung
     * von n^5 auf Gr??enordnung n^3+n^2 gesenkt werden.
     *
     * Hinweis: diese Matritzen sind voll besetzt:
     */
    /*
     	Alle Distanzen zwischen allen Punkten berechnen und in
     	Distanzmatrix speichern Die Berechnung erfolgt mit doubles,
     	das Ergbnis wird auf ganze Meter gerundet ganzzahig
     	gespeichert, um sp?ter schneller damit rechnen zu k?nnen

        Vorsicht: Rechnerabh?ngiger Ganzzahlenbereich mu? 25mal so gro? sein,
        wie die maximale L?nge einer Strecke in Metern!
     */
int OLCOptimizer::initdmval() {
  double d_fak = CONST_D_FAK; /* FAI-Erdradius in metern */

  /* F?r schnellere Berechnung sin/cos-Werte merken */
  double *sinlat = (double*)malloc(sizeof(double)*pnts);
  if (!sinlat)
    return 1;
  double *coslat = (double*)malloc(sizeof(double)*pnts);
  if (!coslat) {
    free(sinlat);
    return 1;
  }
  double *lonrad = (double*)malloc(sizeof(double)*pnts);
  if (!lonrad) {
    free(coslat); free(sinlat);
    return 1;
  }

  double latrad, sli, cli, lri;
  int  j, dist, cmp;

  dmval = (int*)malloc(sizeof(int)*pnts*pnts);
  if (!dmval) {
    free(coslat); free(sinlat); free(lonrad);
    return 1;
  }

  int i;

  for(i=pnts-1;i>=0;i--) {
    /* alle Punkte ins Bogenma? umrechnen und sin/cos Speichern */
    if ((i==pnts-1)&&(project)) {
      lonrad[i] = lon_proj * DEG_TO_RAD;
      latrad = lat_proj * DEG_TO_RAD;
    } else {
      lonrad[i] = lonpnts[rotindex(i)] * DEG_TO_RAD;
      latrad = latpnts[rotindex(i)] * DEG_TO_RAD;
    }
    sinlat[i] = sin(latrad);
    coslat[i] = cos(latrad);
    dmval[i*pnts+i] = 0; /* Diagonale der Matrix mit Distanz 0 f?llen */
  }

  // System.out.println("initializing distances..\n");
  maxdist = 0; /* maximale Distanz zwischen zwei Punkten neu berechnen */
  cmp = pnts-1; /* Schleifenvergleichswert f?r schnelle Berechnung
		   vorher merken */

  // JMW: this calculation is very slow!  N^2 cos!
  for(i=0;i<cmp;i++) { /* diese Schleife NICHT R?CKW?RTS!!! */
    sli = sinlat[i]; cli = coslat[i]; lri = lonrad[i];
    for(j=i+1;j<pnts;j++) {
      if ( (dmval[i*pnts+j] = dist =
	    (int)(d_fak*
		  acos(sli*sinlat[j] + cli*coslat[j]*cos(lri-lonrad[j])
		       )+0.5)  /* auf meter runden */
	    ) > maxdist) {
	maxdist = dist;
      }
      /* ggf. weiteste Distanz merken */
    }
  }

  free(sinlat); free(coslat); free(lonrad);

  // "maximal distance between 2 points: " + maxdist + " meters\n");
  return 0;
}

/*
 * berechne kleinste Distanz dmin(i,j) zwischen allen Punkten x und y mit x<=i und y>=j
 *   f?r alle x<=i, y>=j: dmin(i,j) <= dmin(x,y)
 *      und dmval[dmini[i][j]][dminj[i][j]] <= dmval[x][y]
 */
int OLCOptimizer::initdmin() {
  int i, j, d, mini, minj=0, minimum = maxdist;

  // initializing dmin(i,j) with best start/endpoints for triangles..\n");

  dmin = (int*)malloc(sizeof(int)*pnts*pnts);
  dmini = (int*)malloc(sizeof(int)*pnts*pnts);
  dminj = (int*)malloc(sizeof(int)*pnts*pnts);

  int k, l;

  for(j=pnts-1;j>0;j--) { /* erste Zeile separat behandeln */
    d = dmval[0*pnts+j];
    if (d<minimum) {/* d<=minimum falls gleichwertiger Punkt weiter
		       vorne im track gefunden werden soll */
      minimum = d; minj = j;
    }
    k = j;
    dmin[k]  = minimum;
    dmini[k] = 0;
    dminj[k] = minj;
  }
  for(i=1;i<pnts-1;i++) { /* folgenden Zeilen von vorheriger ableiten */

    // JMW: special case, last point
    j=pnts-1; /* letzte Spalte zur Initialisierung des Minimums
		 getrennt behandeln */

    k = (i-1)*pnts+j;
    minimum = dmin[k];
    mini = dmini[k];
    minj = dminj[k];

    k+= pnts;
    d = dmval[k];
    if (d<minimum) {
      minimum = d; mini = i; minj = j;
    }
    dmin[k]  = minimum;
    dmini[k] = mini;
    dminj[k] = minj;
    for(j=pnts-2;j>i;j--) { /* andere spalten von hinten nach vorne
			       bearbeiten */
      k = i*pnts+j;
      d = dmval[k];
      l = k-pnts;
      if (d<minimum) { /* aktueller Punkt besser als bisheriges Minimum? */
	/* d<=minimum falls gleichwertiger Punkt weiter vorne im track
	   gefunden werden soll */
	minimum = d; mini = i; minj = j;
      }
      if ((d=dmin[l])<minimum) { /* Minimum aus vorheriger Zeile besser? */
	minimum = d; mini = dmini[l]; minj = dminj[l];
      }
      dmin[k]  = minimum;
      dmini[k] = mini;
      dminj[k] = minj;
    }
  }
  return 0;
}

int OLCOptimizer::initmaxend() {
  int w3, i, f, maxf, besti;
  // initializing maxenddist[] with maximal distance to best endpoint ..;
  maxenddist = (int*)malloc(sizeof(int)*pnts);
  if (!maxenddist) return 1;

  maxendpunkt = (int*)malloc(sizeof(int)*pnts);
  if (!maxendpunkt) {
    free(maxendpunkt);
    return 1;
  }

  for(w3=pnts-1; w3>1; w3--) {
    maxf = 0;
    for(i=besti=pnts-1; i>=w3; i--) {
      if ((f = dmval[w3*pnts+i])>maxf) {
	maxf = f; besti = i;
      }
    }
    maxenddist[w3]  = maxf;
    maxendpunkt[w3] = besti;
  }
  return 0;
}

void OLCOptimizer::addPoint(double lon, double lat, double alt, double time) {
  static double lonlast;
  static double latlast;

  if (pnts_in==0) {
    latlast = lat;
    lonlast = lon;
  }
  if ((Distance(lat, lon, latlast, lonlast)>DISTANCETHRESHOLD*1000.0)
      || (pnts_in==0)) {
    latlast = lat;
    lonlast = lon;

    int i = (pnts_in + pnts_offset_in) % MAXPOINTS;

    timepnts[i] = (long)time;
    latpnts[i] = lat;
    lonpnts[i] = lon;
    altpnts[i] = (int)alt;
    if (pnts_in<MAXPOINTS) {
      pnts_in++;
      pnts_offset_in = 0;
    } else {
      pnts_offset_in++;
      pnts_offset_in %= MAXPOINTS;
    }

  }
};


int OLCOptimizer::getN() {
  return pnts;
}

double OLCOptimizer::getLatitude(int i) {
  return latpnts[rotindex(i)];
}

double OLCOptimizer::getLongitude(int i) {
  return lonpnts[rotindex(i)];
}


void OLCOptimizer::SetLine() {

  LockFlightData();
  pnts = pnts_in; // save value in case we get new data while
		  // performing the analysis/display
  pnts_offset = pnts_offset_in;
  UnlockFlightData();

}


bool OLCOptimizer::OptimizeProjection(double lon, double lat, double alt)
{
  LockFlightData();
  pnts = pnts_in; // save value in case we get new data while
		  // performing the analysis
  pnts_offset = pnts_offset_in;

  lat_proj = lat;
  lon_proj = lon;
  alt_proj = (int)alt;

  project = true;

  if (pnts<MAXPOINTS) {
    pnts++;
    pnts_offset = 0;
  } else {
    pnts_offset++;
    pnts_offset %= MAXPOINTS;
  }

  UnlockFlightData();

  DWORD tm =GetTickCount();

  bool retval = (optimize_internal() == 0);
  Clear();

#ifdef DEBUG
  if (retval) {
    char buffer[200];

    sprintf(buffer,"%d %d # OLC time\n",
	    pnts, GetTickCount()-tm);
    DebugStore(buffer);

    sprintf(buffer,"%d  %d %d %d %d %d # OLC max12345\n",
	    maxroute, max1, max2, max3, max4, max5);
    DebugStore(buffer);
    sprintf(buffer,"%d  %d %d %d %d %d # OLC max fai\n",
	    bestfai, max1fai, max2fai, max3fai, max4fai, max5fai);
    DebugStore(buffer);
    sprintf(buffer,"%d  %d %d %d %d %d # OLC max flach\n",
	    bestflach, max1flach, max2flach, max3flach, max4flach, max5flach);
    DebugStore(buffer);
  }
#endif

  return retval;

}


bool OLCOptimizer::Optimize() {
  SetLine();

  project = false;

  DWORD tm =GetTickCount();

  bool retval = (optimize_internal() == 0);
  Clear();

#ifdef DEBUG
  if (retval) {

    char buffer[200];

    sprintf(buffer,"%d %d # OLC time\n",
	    pnts, GetTickCount()-tm);
    DebugStore(buffer);

    sprintf(buffer,"%d  %d %d %d %d %d # OLC max12345\n",
	    maxroute, max1, max2, max3, max4, max5);
    DebugStore(buffer);
    sprintf(buffer,"%d  %d %d %d %d %d # OLC max fai\n",
	    bestfai, max1fai, max2fai, max3fai, max4fai, max5fai);
    DebugStore(buffer);
    sprintf(buffer,"%d  %d %d %d %d %d # OLC max flach\n",
	    bestflach, max1flach, max2flach, max3flach, max4flach, max5flach);
    DebugStore(buffer);

    sprintf(buffer,"%f %f # OLC pos 1\n",
	    lonpnts[rotindex(max1flach)], latpnts[rotindex(max1flach)]);
    DebugStore(buffer);

    sprintf(buffer,"%f %f # OLC pos 2\n",
	    lonpnts[rotindex(max2flach)], latpnts[rotindex(max2flach)]);
    DebugStore(buffer);

    sprintf(buffer,"%f %f # OLC pos 3\n",
	    lonpnts[rotindex(max3flach)], latpnts[rotindex(max3flach)]);
    DebugStore(buffer);

    sprintf(buffer,"%f %f # OLC pos 4\n",
	    lonpnts[rotindex(max4flach)], latpnts[rotindex(max4flach)]);
    DebugStore(buffer);

    sprintf(buffer,"%f %f # OLC pos 5\n",
	    lonpnts[rotindex(max5flach)], latpnts[rotindex(max5flach)]);
    DebugStore(buffer);
  }
#endif

  return retval;
}


void OLCOptimizer::UpdateSolution(int dbest,
				  int p1, int p2, int p3, int p4, int p5,
				  OLCSolution* solution) {
  bool improved = !(solution->valid);
  if (dbest>0) {
    if (dbest> solution->distance) {
      improved = true;
    }
    if (improved) {
      solution->valid = true;
      solution->distance = dbest;
      solution->latitude[0] = latpnts[rotindex(p1)];
      solution->longitude[0] = lonpnts[rotindex(p1)];
      solution->latitude[1] = latpnts[rotindex(p2)];
      solution->longitude[1] = lonpnts[rotindex(p2)];
      solution->latitude[2] = latpnts[rotindex(p3)];
      solution->longitude[2] = lonpnts[rotindex(p3)];
      solution->latitude[3] = latpnts[rotindex(p4)];
      solution->longitude[3] = lonpnts[rotindex(p4)];
      solution->latitude[4] = latpnts[rotindex(p5)];
      solution->longitude[4] = lonpnts[rotindex(p5)];
      solution->time = timepnts[rotindex(p5)]-timepnts[rotindex(p1)];
    }
  }
}

int OLCOptimizer::optimize_internal() {

  int i1, i3, i4;
  int i, a, b, c, d, e, u, w, tmp, aplusb;
  int i4cmp, i2cmp = pnts-2;

  if (pnts<5) {
    // only <5 points given, no optimization
    return 0;
  }
  if( initdmval() != 0 ) return 1;
  if( initdmin() != 0 ) return 1;
  if( initmaxend() != 0 ) return 1;

  max1 = max2 = max3 = max4 = max5 = maxroute= bestfai = bestflach = 0;
  // calculating best waypoints.. for more than 500 points need some
  // time..
  int k24;
  for(i2=0; i2<i2cmp; i2++) {
    if(stop) break;

    /* 1.Wende */
    for(i=i1=e=0; i<i2; i++) {
      /* Startpunkt f?r freie Strecke separat optimieren */
      if ((tmp = dmval[i*pnts+i2])>e) { e = tmp; i1 = i; }
    } /* e, i1 enthalten fuer dieses i2 den besten Wert */

    i4cmp = i2+2;

    for(i4=pnts; --i4>=i4cmp;) { /* 3.Wende von hinten optimieren */
      k24 = i2*pnts+i4;
      aplusb = 0;

      int start = dmini[k24];
      int finish = dminj[k24];
      int dh, dt;

      for(i=i3=i2+1; i<i4; i++) { /* 2.Wende separat optimieren */

	if ((tmp=(a=dmval[i2*pnts+i])+(b=dmval[i*pnts+i4]))>aplusb) {
	  /* findet gr??tes a+b (und auch gr??tes Dreieck) */
	  aplusb = tmp; i3 = i;
	}
	// TODO: modified rules if >500 km

	int d5 = (d = dmin[k24])*5;
	int a25 = a*25;
	int b25 = b*25;
	int c25 = (c = dmval[k24])*25;

	if (d5<=(u=tmp+c)) { /* Dreieck gefunden 5*d<= a+b+c */

	  start = dmini[k24];
	  finish = dminj[k24];
	  if (project) {
	    // JMW fudge
	    dh=0;
	    dt=0;
	  } else {
	    dh = altpnts[rotindex(finish)]-altpnts[rotindex(start)];
	    dt = timepnts[rotindex(finish)]-timepnts[rotindex(start)];
	  }

	  // only valid if dh>0 (finish higher than start)
	  // only valid if dt<2.5 hours (9000 seconds)?
	  // currently unused.

	  if ((c25>=(tmp=u*7))&&(a25>=tmp)&&(b25>=tmp)) {
	    /* FAI-D gefunden */
	    if (dh>= -1000) { // no time limit
	      if ((w=u-d)>bestfai) { /* besseres FAI-D gefunden */
		max1fai = start;
		max2fai = i2;
		max3fai = i;
		max4fai = i4;
		max5fai = finish;
		bestfai = w;
	      }
	    }
	    //	      }
	  } else { /* nicht FAI=flaches Dreieck gefunden.
		      Non-FAI 'flat' triangle found
		   */
	    if ((w=u-d)>bestflach) {
	      max1flach = start;
	      max2flach = i2;
	      max3flach = i;
	      max4flach = i4;
	      max5flach = finish;
	      bestflach = w;
	    }
	  }
	}
      } /* aplusb, i3 enthalten fuer dieses i2 und i4 besten Wert */

      if ((tmp = maxenddist[i4]+aplusb+e) > maxroute) {
	// max sprint (OLC World League)
	start = i1;
	finish = maxendpunkt[i4];
	dh = altpnts[rotindex(finish)]-altpnts[rotindex(start)];
	dt = timepnts[rotindex(finish)]-timepnts[rotindex(start)];
	if ((dh>=0)&&(dt<=9000)) {
	  max1 = start; max2 = i2; max3 = i3; max4 = i4; max5 = finish;
	  maxroute = tmp;
	}
      }


    }
  }

  // World league (sprint):
  //   start height <= finish height
  //   2.5 hours max

  // FAI triangle
  //   start height <= finish height+1000m

  UpdateSolution(bestfai, max1fai, max2fai, max3fai, max4fai, max5fai,
		 &solution_FAI_triangle);
  UpdateSolution(maxroute, max1, max2, max3, max4, max5,
		 &solution_FAI_sprint);

  return 0;
}



/*
  TODO:

  Analysis page for OLC:
    button to perform analysis,
    selector property to select which type of task to display
    handicap in glide polar?

  Estimate score?
    check limits: start altitude, max time.

  Rotary buffer, and save previous best when doing optimisation.
  This means we only need a buffer large enough for 2.5 hours,
  so we can plan on update every one minute seconds for 200 samples max
  (allowing small margin).
  This gives optimisation time of 2 seconds.
  At 100 knots, one minute = 3.3 km.  Set distance threshold to 1 km.

*/
