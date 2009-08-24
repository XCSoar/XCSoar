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

#include "BestAlternate.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "WayPoint.hpp"
#include "Math/FastMath.h"
#include "externs.h"
#include "McReady.h"
#include "Dialogs.h"
#include "GlideSolvers.hpp"
#include "Utils.h"
#include "Math/Earth.hpp"

// in Calculations.cpp
void LatLon2Flat(double lon, double lat, int *scx, int *scy);

int CalculateWaypointApproxDistance(int scx_aircraft, int scy_aircraft,
                                    int i);


/*
 * ===========================================
 * VENTA3 SearchBestAlternate() beta
 * based on SortLandableWaypoints and extended
 * by Paolo Ventafridda
 * ===========================================
 */
#ifdef DEBUG
#define DEBUG_BESTALTERNATE
#endif
#define MAXBEST 10      // max number of reachable landing points
			// searched for, among a preliminar list of
			// MAXBEST * 2 - CPU HOGGING ALERT!

void SearchBestAlternate(NMEA_INFO *Basic,
			 DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXBEST];
  double SortedArrivalAltitude[MAXBEST];
  int SortedApproxDistance[MAXBEST*2];
  int SortedApproxIndex[MAXBEST*2];
  int i, k, l;
  double arrival_altitude;
  int active_bestalternate_on_entry=-1;
  int bestalternate=-1;

#ifdef DEBUG_BESTALTERNATE
  TCHAR ventabuffer[200];
#endif

  if (!WayPointList) return;

  /*
   * VENTA3 search in range of optimum gliding capability
   * and in any case within an acceptable distance, say 100km.
   * Anything else is not considered, since we want a safe landing not a long glide.
   * Preferred waypoints and home are nevertheless checked in any case later.
   * Notice that if you are over 100km away from the nearest non-preferred landing point you can't
   * expect a computer to be helpful in case of troubles.
   *
   * ApproxDistance is in km, very approximate
   */

  double searchrange=(GPS_INFO.Altitude-SAFETYALTITUDEARRIVAL)* GlidePolar::bestld /1000;
  if (searchrange <= 0)
    searchrange=2; // lock to home airport at once
  if (searchrange > ALTERNATE_MAXRANGE)
    searchrange=ALTERNATE_MAXRANGE;

  LockTaskData();
  active_bestalternate_on_entry = BestAlternate;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  // Clear search lists
  for (i=0; i<MAXBEST*2; i++) {
    SortedApproxIndex[i]= -1;
    SortedApproxDistance[i] = 0;
  }
  for (i=0; i<(int)NumberOfWayPoints; i++) {

    if (!(((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
          ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    int approx_distance =
      CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, i);

    // Size a reasonable distance, wide enough VENTA3
    if ( approx_distance > searchrange ) continue;

    // see if this fits into slot
    for (k=0; k< MAXBEST*2; k++)  {

      if (((approx_distance < SortedApproxDistance[k])
           // wp is closer than this one
	   || (SortedApproxIndex[k]== -1))   // or this one isn't filled
          && (SortedApproxIndex[k]!= i))    // and not replacing with same
        {
	  // ok, got new biggest, put it into the slot.
          for (l=MAXBEST*2-1; l>k; l--) {
            if (l>0) {
	      SortedApproxDistance[l] = SortedApproxDistance[l-1];
	      SortedApproxIndex[l] = SortedApproxIndex[l-1];
            }
          }

          SortedApproxDistance[k] = approx_distance;
          SortedApproxIndex[k] = i;
          k=MAXBEST*2;
        }
    } // for k
  } // for i

#ifdef DEBUG_BESTALTERNATE
  FILE *fp;
  if ( (fp=_tfopen(_T("DEBUG.TXT"),_T("a"))) != NULL )  {
    _stprintf(ventabuffer,TEXT("==================\n"));
    fprintf(fp,"%S",ventabuffer);
    _stprintf(ventabuffer,TEXT("[GPSTIME=%02d:%02d:%02d] Altitude=%dm searchrange=%dKm Curr.Best=%d\n\n"),
	     GPS_INFO.Hour, GPS_INFO.Minute, GPS_INFO.Second,
	     (int)GPS_INFO.Altitude, (int)searchrange, BestAlternate);
    fprintf(fp,"%S",ventabuffer);
    for ( int dbug=0; dbug<MAXBEST*2; dbug++) {
      if ( SortedApproxIndex[dbug] <0 ) _stprintf(ventabuffer,_T("%d=empty\n"), dbug);
      else
        _stprintf(ventabuffer,TEXT("%d=%s(%d)\n"), dbug,
		 WayPointList[SortedApproxIndex[dbug]].Name, SortedApproxDistance[dbug] );
      fprintf(fp,"%S",ventabuffer);
    }
    fclose(fp);
  } else
    DoStatusMessage(_T("CANNOT OPEN DEBUG FILE"));
#endif


  // Now do detailed search
  for (i=0; i<MAXBEST; i++) {
    SortedLandableIndex[i]= -1;
    SortedArrivalAltitude[i] = 0;
  }

  bool found_reachable_airport = false;

  for (int scan_airports_slot=0;
       scan_airports_slot<2;
       scan_airports_slot++) {

    if (found_reachable_airport ) {
      continue; // don't bother filling the rest of the list
    }

    for (i=0; i<MAXBEST*2; i++) {
      if (SortedApproxIndex[i]<0) { // ignore invalid points
        continue;
      }

      if ((scan_airports_slot==0) &&
	  ((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic,
                                         Calculated,
                                         SortedApproxIndex[i]);

      WayPointCalc[SortedApproxIndex[i]].AltArriv = arrival_altitude;
      // This is holding the real arrival value

      /*
       * We can't use degraded polar here, but we can't accept an
       * arrival 1m over safety.  That is 2m away from being
       * unreachable! So we higher this value to 100m.
       */
      arrival_altitude -= ALTERNATE_OVERSAFETY;

      if (scan_airports_slot==0) {
        if (arrival_altitude<0) {
          // in first scan, this airport is unreachable, so ignore it.
          continue;
        } else
          // this airport is reachable
          found_reachable_airport = true;
      }

      // see if this fits into slot
      for (k=0; k< MAXBEST; k++) {
        if (((arrival_altitude > SortedArrivalAltitude[k])
             // closer than this one
             ||(SortedLandableIndex[k]== -1))
            // or this one isn't filled
	    &&(SortedLandableIndex[k]!= i))  // and not replacing
	  // with same
          {
	    /*
	      #ifdef DEBUG_BESTALTERNATE
	      _stprintf(ventabuffer,TEXT("SAI[i=%d]=%s   SLI[k=%d]=n%d"), i, WayPointList[SortedApproxIndex[i]].Name,
	      k, SortedLandableIndex[k] );
	      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	        {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	      #endif
	    */
            double wp_distance, wp_bearing;
            DistanceBearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude,
                            &wp_distance, &wp_bearing);

	    WayPointCalc[SortedApproxIndex[i]].Distance = wp_distance;
	    WayPointCalc[SortedApproxIndex[i]].Bearing = wp_bearing;

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, Basic, Calculated,
                                       NULL,
                                       NULL,
                                       wp_distance,
                                       &out_of_range, NULL);

            if ((distance_soarable>= wp_distance)||(arrival_altitude<0)) {
              // only put this in the index if it is reachable
              // and doesn't go through terrain, OR, if it is unreachable
              // it doesn't matter if it goes through terrain because
              // pilot has to climb first anyway

              // ok, got new biggest, put it into the slot.
              for (l=MAXBEST-1; l>k; l--) {
                if (l>0) {
                  SortedArrivalAltitude[l] = SortedArrivalAltitude[l-1];
                  SortedLandableIndex[l] = SortedLandableIndex[l-1];
                }
              }

              SortedArrivalAltitude[k] = arrival_altitude;
              SortedLandableIndex[k] = SortedApproxIndex[i];
              k=MAXBEST;
            }
          } // if (((arrival_altitude > SortedArrivalAltitude[k]) ...
	/*
	  #ifdef DEBUG_BESTALTERNATE
	  else {
	  _stprintf(ventabuffer,TEXT("  SAI[i=%d]=%s   SLI[k=%d]=n%d"), i, WayPointList[SortedApproxIndex[i]].Name,
	  k, SortedLandableIndex[k] );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}

	  }
	  #endif
	*/
      } // for (k=0; k< MAXBEST; k++) {
    }
  }

  // extended part by Paolo

#ifdef DEBUG_BESTALTERNATE
  if ( (fp=_tfopen(_T("DEBUG.TXT"),_T("a"))) != NULL )  {
    _stprintf(ventabuffer,_T("\nLandable:\n"));
    fprintf(fp,"%S",ventabuffer);
    for ( int dbug=0; dbug<MAXBEST; dbug++) {
      if ( SortedLandableIndex[dbug] <0 ) {
	_stprintf(ventabuffer,_T("%d=empty\n"), dbug);
	fprintf(fp,"%S",ventabuffer);
      }
      else
	{
          _stprintf(ventabuffer,_T("%d=%s (%dm)"), dbug,
		   WayPointList[SortedLandableIndex[dbug]].Name, (int)SortedArrivalAltitude[dbug] );
	  fprintf(fp,"%S",ventabuffer);
	  if ( SortedLandableIndex[dbug] == HomeWaypoint )
	    _stprintf(ventabuffer,_T(":HOME") );
	  else
	    if ( WayPointCalc[SortedLandableIndex[dbug]].Preferred == TRUE )
	      _stprintf(ventabuffer,_T(":PREF") );
	    else
	      _stprintf(ventabuffer,_T("") );
	  fprintf(fp,"%S\n",ventabuffer);
	}

    }
    fclose(fp);
  } else
    DoStatusMessage(_T("CANNOT OPEN DEBUG FILE"));
#endif

  bestalternate=-1;  // reset the good choice
  double safecalc = Calculated->NavAltitude - SAFETYALTITUDEARRIVAL;
  static double grpolar = GlidePolar::bestld *SAFELD_FACTOR;
  int curwp, curbestairport=-1, curbestoutlanding=-1;
  double curgr=0, curbestgr=INVALID_GR;
  if ( safecalc <= 0 ) {
    /*
     * We're under the absolute safety altitude at MSL, can't be any better elsewhere!
     * Use the closer, hopefully you are landing on your airport
     */
#ifdef DEBUG_BESTALTERNATE
    _stprintf(ventabuffer,TEXT("Under safety at MSL, using closer"));
    if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
    // DoStatusMessage(ventabuffer);
#endif

  } else
    for (k=0;  k< MAXBEST; k++) {
      curwp = SortedLandableIndex[k];

      if ( !ValidWayPoint(curwp) ) {
	//#ifdef DEBUG_BESTALTERNATE
	//		_stprintf(ventabuffer,TEXT("k=%d skip invalid waypoint curwp=%d"), k, curwp );
	//		if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	//#endif
	continue;
	// break;  // that list is unsorted !
      }

      // At the first unsafe landing, stop searching down the list and use the best found or the first
      double grsafe=safecalc - WayPointList[curwp].Altitude;
      if ( grsafe <= 0 ) {
	/*
	 * We're under the safety altitude for this waypoint.
	 */
	/*
	  #ifdef DEBUG_BESTALTERNATE
	  _stprintf(ventabuffer,TEXT("N.%d=%s under safety."),k, WayPointList[curwp].Name);
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	  DoStatusMessage(ventabuffer);
	  #endif
	*/
	break;
	//continue;
      }

      WayPointCalc[curwp].GR = WayPointCalc[curwp].Distance / grsafe; grsafe = WayPointCalc[curwp].GR;
      curgr=WayPointCalc[curwp].GR;

      if ( grsafe > grpolar ) {
	/*
	 * Over degraded polar GR for this waypoint
	 */
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("k=%d %s grsafe=%d > grpolar=%d, skipping. "),
		 k, WayPointList[curwp].Name, (int)grsafe, (int)grpolar );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	continue;
	// break;
      }

      // Anything now is within degraded glide ratio, so if our homewaypoint is safely reachable then
      // attempt to lock it even if we already have a valid best, even if it is preferred and even
      // if it has a better GR

      if ( (HomeWaypoint >= 0) && (curwp == HomeWaypoint) ) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("k=%d locking Home"), k);
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	bestalternate = curwp;
	break;
      }

      // If we already found a preferred, stop searching for anything but home

      if ( bestalternate >= 0 && WayPointCalc[bestalternate].Preferred) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("Ignoring:[k=%d]%s because current best <%s> is a PREF"), k,
		 WayPointList[curwp].Name, WayPointList[bestalternate].Name);
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	continue;
      }

      // VENTA5 TODO: extend search on other preferred, choosing the closer one

      // Preferred list has priority, first found is taken (could be smarted)

      if ( WayPointCalc[ curwp ].Preferred ) {
	bestalternate=curwp;
#ifdef DEBUG_BESTALTERNATE
	_stprintf(ventabuffer,TEXT("k=%d PREFERRED bestalternate=%d,%s"), k,curwp,
		 WayPointList[curwp].Name );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	// DoStatusMessage(ventabuffer);
#endif
	continue;
      }

      // else we remember the best landable GR found so far. We shall use this in case
      // at the end of the search no home and no preferred were found.

      if ( curgr < curbestgr ) {
	if ( ( WayPointList[curwp].Flags & AIRPORT) == AIRPORT) {
	  curbestairport=curwp;
	  curbestgr=curgr; // ONLY FOR AIRPORT! NOT FOR OUTLANDINGS!!
#ifdef DEBUG_BESTALTERNATE
          _stprintf(ventabuffer,TEXT("[k=%d]<%s> (curgr=%d < curbestgr=%d) set as bestairport"),
		   k, WayPointList[curwp].Name, (int)curgr, (int)curbestgr );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	}
	else {
	  curbestoutlanding=curwp;
#ifdef DEBUG_BESTALTERNATE
          _stprintf(ventabuffer,TEXT("[k=%d]<%s> (curgr=%d < curbestgr=%d) set as bestoutlanding"),
		   k, WayPointList[curwp].Name, (int)curgr, (int)curbestgr );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	}
      }

      continue;

    }

  if ( bestalternate <0 ) {

    if ( curbestairport >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("--> no bestalternate, choosing airport <%s> with gr=%d"),
	       WayPointList[curbestairport].Name, (int)curbestgr );
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
      // DoStatusMessage(ventabuffer);
#endif
      bestalternate=curbestairport;
    } else {
      if ( curbestoutlanding >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("--> no bestalternate, choosing outlanding <%s> with gr=%d"),
		 WayPointList[curbestoutlanding].Name, (int)curbestgr );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	// DoStatusMessage(ventabuffer);
#endif
	bestalternate=curbestoutlanding;
      } else {
	/*
	 * Here we are in troubles, nothing really reachable, but we
	 * might still be lucky to be within the "yellow" glide
	 * path. In any case we select the best arrival altitude place
	 * available, even if it is "red".
	 */
	if ( ValidWayPoint(SortedLandableIndex[0]) ) {
	  bestalternate=SortedLandableIndex[0];
#ifdef DEBUG_BESTALTERNATE
	  _stprintf(ventabuffer,TEXT("--> No bestalternate was found, and no good airport or outlanding!\n    Setting first available: <%s>"),
		   WayPointList[bestalternate].Name);
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	  // DoStatusMessage(ventabuffer);
#endif
	} else {
	  /*
	   * Else the Landable list is EMPTY, although we might be
	   * near to a landable point but the terrain obstacles look
	   * too high (or the DEM resolution is not high enough to
	   * show a passage).
	   *
	   * Still the old BestAlternate could simply be out of range,
	   * but reachable...  These values have certainly just been
	   * calculated by DoAlternates() , so they are usable.
	   */
	  // Attempt to use the old best, but check there's one.. it
	  // might be empty for the first run
	  if ( ValidWayPoint(active_bestalternate_on_entry) )
	    {
	      bestalternate=active_bestalternate_on_entry;
	      if ( WayPointCalc[bestalternate].AltArriv <0 ) {
#ifdef DEBUG_BESTALTERNATE
		_stprintf(ventabuffer,TEXT("Landable list is empty and old bestalternate <%s> has Arrival=%d <0, NO good."),
			 WayPointList[bestalternate].Name, (int) WayPointCalc[bestalternate].AltArriv);
		if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		// Pick up the closest!
		if ( ValidWayPoint( SortedApproxIndex[0]) ) {
		  bestalternate=SortedApproxIndex[0];
#ifdef DEBUG_BESTALTERNATE
		  _stprintf(ventabuffer,
			   TEXT(".. using the closer point found: <%s> distance=~%d Km, you need to climb!"),
			   WayPointList[bestalternate].Name, SortedApproxDistance[0]);
		  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		} else {
		  /// CRITIC POINT
		  // Otherwise ..
		  // nothing, either keep the old best or set it empty
		  // Put here "PULL-UP! PULL-UP! Boeing cockpit voice sound and possibly shake the stick.
#ifdef DEBUG_BESTALTERNATE
		  _stprintf(ventabuffer,TEXT("Out of ideas..good luck"));
		  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		}
	      } else
		{
		  // MapWindow2 is checking for reachables separately,
		  // se let's see if this closest is reachable
		  if ( ValidWayPoint( SortedApproxIndex[0] )) {
		    if ( WayPointList[SortedApproxIndex[0]].Reachable ) {
		      bestalternate = SortedApproxIndex[0];
#ifdef DEBUG_BESTALTERNATE
		      _stprintf(ventabuffer,TEXT("Closer point found: <%s> distance=~%d Km, Reachable with arrival at %d!"),
			       WayPointList[bestalternate].Name, SortedApproxDistance[0], (int) WayPointList[bestalternate].AltArivalAGL);
		      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		    } else
		      {
#ifdef DEBUG_BESTALTERNATE
			_stprintf(ventabuffer,TEXT("Closer point found: <%s> distance=~%d Km, UNReachable"),
				 WayPointList[bestalternate].Name, SortedApproxDistance[0]);
			if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		      }
		  } else
		    {
#ifdef DEBUG_BESTALTERNATE
		      _stprintf(ventabuffer,TEXT("Landable list is empty, no Closer Approx, but old best %s is still reachable (arrival=%d)"),
			       WayPointList[bestalternate].Name, (int)WayPointCalc[bestalternate].AltArriv);
		      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		    }
		}
	    } else
	    {
	      /// CRITIC POINT
#ifdef DEBUG_BESTALTERNATE
	      _stprintf(ventabuffer,TEXT("Landable list is empty, and NO valid old bestalternate"));
	      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	    }
	}
	/*
	 * Don't make any sound at low altitudes, pilot is either taking off
	 * or landing, or searching for an immediate outlanding.  Do not disturb.
	 * If safetyaltitude is 300m, then below 500m be quiet.
	 * If there was no active alternate on entry, and nothing was found, then we
	 * better be quiet since probably the user had already been alerted previously
	 * and now he is low..
	 */
	if ( bestalternate >0 &&
	     ((safecalc-WayPointList[bestalternate].Altitude) >ALTERNATE_QUIETMARGIN)) {
	  if ( WayPointList[bestalternate].AltArivalAGL <100 )
	    AlertBestAlternate(2);
	  //	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_RED"));
	}
      }
    }
  }

  /*
   * If still invalid, it should mean we are just taking off
   * in this case no problems, we set the very first bestalternate of the day as the home
   * trusting the user to be home really!
   */

  if ( bestalternate < 0 ) {
    if ( HomeWaypoint >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("BESTALTERNATE HOME (%s)"),
	       WayPointList[HomeWaypoint].Name );
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
      //DoStatusMessage(ventabuffer);
#endif
      bestalternate=HomeWaypoint;
    }
  } else {
    // If still invalid, i.e. not -1, then there's a big problem
    if ( !ValidWayPoint(bestalternate) ) {
      //if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_RED"));
      AlertBestAlternate(2);
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("WARNING ERROR INVALID BEST=%d"),bestalternate);
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
      DoStatusMessage(_T("WARNING ERROR INVALID BEST!"));
      // todo: immediate disable function
    }
  }

  if (active_bestalternate_on_entry != bestalternate) {
    BestAlternate = bestalternate;
    if ( bestalternate >0 && ((safecalc-WayPointList[bestalternate].Altitude) >ALTERNATE_QUIETMARGIN))
      AlertBestAlternate(1);
    //		if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_GREEN"));
  }

  UnlockTaskData();
}

/*
 * Do not disturb too much. Play alert sound only once every x minutes, not more.
 */
void AlertBestAlternate(short soundmode) {
#ifdef DEBUG_BESTALTERNATE
  TCHAR ventabuffer[100];
  FILE *fp;
#endif

  static double LastAlertTime=0;

  if ( GPS_INFO.Time > LastAlertTime + 180.0 ) {
    if (EnableSoundModes) {
      LastAlertTime = GPS_INFO.Time;
      switch (soundmode) {
      case 0:
	break;
      case 1:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_GREEN"));
#endif
	break;
      case 2:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_RED"));
#endif
	break;
      case 11:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_GREEN"));
	PlayResource(TEXT("IDR_WAV_GREEN"));
#endif
	break;
      default:
	break;
      }
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("(PLAYED ALERT SOUND, soundmode=%d)"), soundmode);
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
    }
  } else {
#ifdef DEBUG_BESTALTERNATE
    _stprintf(ventabuffer,TEXT("(QUIET, NO PLAY ALERT SOUND, soundmode=%d)"), soundmode);
    if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
      {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
  }
}

void DoBestAlternateSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static double LastSearchBestTime = 0; // VENTA3

 // VENTA3 best landing slow calculation
#if (WINDOWSPC>0)
  if ( (OnBestAlternate == true ) && (Basic->Time > LastSearchBestTime+10.0) ) // VENTA3
#else
  if ( (OnBestAlternate == true ) && (Basic->Time > LastSearchBestTime+BESTALTERNATEINTERVAL) ) // VENTA3
#endif
    {
      LastSearchBestTime = Basic->Time;
      SearchBestAlternate(Basic, Calculated);
    }

}


/*
 * VENTA3 Alternates destinations
 *
 * Used by Alternates and BestAlternate
 *
 * Colors VGR are disabled, but available
 */

void DoAlternates(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int AltWaypoint) { // VENTA3
  if (!ValidWayPoint(AltWaypoint)) {
    return;
  }
  double w1lat = WayPointList[AltWaypoint].Latitude;
  double w1lon = WayPointList[AltWaypoint].Longitude;
  double w0lat = Basic->Latitude;
  double w0lon = Basic->Longitude;
  double *altwp_dist = &WayPointCalc[AltWaypoint].Distance;
  double *altwp_gr   = &WayPointCalc[AltWaypoint].GR;
  double *altwp_arrival = &WayPointCalc[AltWaypoint].AltArriv;
  short  *altwp_vgr  = &WayPointCalc[AltWaypoint].VGR;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  altwp_dist, NULL);

  double GRsafecalc = Calculated->NavAltitude - (WayPointList[AltWaypoint].Altitude + SAFETYALTITUDEARRIVAL);

  if (GRsafecalc <=0) *altwp_gr = INVALID_GR;
  else {
	*altwp_gr = *altwp_dist / GRsafecalc;
	if ( *altwp_gr >ALTERNATE_MAXVALIDGR || *altwp_gr <0 ) *altwp_gr = INVALID_GR;
	else if ( *altwp_gr <1 ) *altwp_gr = 1;
  }


  // We need to calculate arrival also for BestAlternate, since the last "reachable" could be
  // even 60 seconds old and things may have changed drastically

  *altwp_arrival = CalculateWaypointArrivalAltitude(Basic, Calculated, AltWaypoint);
  if ( (*altwp_arrival - ALTERNATE_OVERSAFETY) >0 ) {
  	if ( *altwp_gr <= (GlidePolar::bestld *SAFELD_FACTOR) ) *altwp_vgr = 1; // full green vgr
  	else
  		if ( *altwp_gr <= GlidePolar::bestld ) *altwp_vgr = 2; // yellow vgr
		else *altwp_vgr =3; // RED vgr
  } else
  {
	*altwp_vgr = 3; // full red
  }


}
