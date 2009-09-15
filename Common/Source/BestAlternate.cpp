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

#include "GlideComputerTask.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "WayPoint.hpp"
#include "Math/FastMath.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "McReady.h"
#include "Message.h"
#include "GlideSolvers.hpp"
#include "Audio/Sound.hpp"
#include "Math/Earth.hpp"
#include "Abort.hpp"
#include "Components.hpp"
#include "WayPointList.hpp"


/*
 * ===========================================
 * VENTA3 SearchBestAlternate() beta
 * based on SortLandableWaypoints and extended
 * by Paolo Ventafridda
 * ===========================================
 */

#define MAXBEST 10      // max number of reachable landing points
			// searched for, among a preliminar list of
			// MAXBEST * 2 - CPU HOGGING ALERT!

void
GlideComputerTask::SearchBestAlternate()
{
  int SortedLandableIndex[MAXBEST];
  double SortedArrivalAltitude[MAXBEST];
  int SortedApproxDistance[MAXBEST*2];
  int SortedApproxIndex[MAXBEST*2];
  int i, k, l;
  double arrival_altitude;
  int active_bestalternate_on_entry=-1;
  int bestalternate=-1;

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

  double searchrange=(Basic().Altitude-
		      SettingsComputer().SAFETYALTITUDEARRIVAL)
    *GlidePolar::bestld /1000;
  if (searchrange <= 0)
    searchrange=2; // lock to home airport at once
  if (searchrange > ALTERNATE_MAXRANGE)
    searchrange=ALTERNATE_MAXRANGE;

  mutexTaskData.Lock();
  active_bestalternate_on_entry = Calculated().BestAlternate;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic().Location, &scx_aircraft, &scy_aircraft);

  // Clear search lists
  for (i=0; i<MAXBEST*2; i++) {
    SortedApproxIndex[i]= -1;
    SortedApproxDistance[i] = 0;
  }

  for (i = 0; way_points.verify_index(i); i++) {
    const WAYPOINT &way_point = way_points.get(i);

    if (!(((way_point.Flags & AIRPORT) == AIRPORT) ||
          ((way_point.Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    int approx_distance =
      CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, way_point);

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

      const WAYPOINT &way_point = way_points.get(SortedApproxIndex[i]);
      WPCALC &wpcalc = way_points.set_calc(SortedApproxIndex[i]);

      if ((scan_airports_slot==0) &&
	  ((way_point.Flags & AIRPORT) != AIRPORT)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude = CalculateWaypointArrivalAltitude(way_point, wpcalc);

      wpcalc.AltArrival = arrival_altitude;
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
            double wp_distance, wp_bearing;
            DistanceBearing(Basic().Location, way_point.Location,
                            &wp_distance, &wp_bearing);

            wpcalc.Distance = wp_distance;
            wpcalc.Bearing = wp_bearing;

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, 
				       &Basic(), &Calculated(),
				       SettingsComputer(),
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
      } // for (k=0; k< MAXBEST; k++) {
    }
  }

  // extended part by Paolo

  bestalternate=-1;  // reset the good choice
  double safecalc = Calculated().NavAltitude - 
    SettingsComputer().SAFETYALTITUDEARRIVAL;
  static double grpolar = GlidePolar::bestld *SAFELD_FACTOR;
  int curwp, curbestairport=-1, curbestoutlanding=-1;
  double curgr=0, curbestgr=INVALID_GR;
  if ( safecalc <= 0 ) {
    /*
     * We're under the absolute safety altitude at MSL, can't be any better elsewhere!
     * Use the closer, hopefully you are landing on your airport
     */
  } else
    for (k=0;  k< MAXBEST; k++) {
      curwp = SortedLandableIndex[k];

      if ( !ValidWayPoint(curwp) ) {
	continue;
	// break;  // that list is unsorted !
      }

      const WAYPOINT &way_point = way_points.get(curwp);
      WPCALC &wpcalc = way_points.set_calc(SortedApproxIndex[i]);

      // At the first unsafe landing, stop searching down the list and
      // use the best found or the first
      double grsafe = safecalc - way_point.Altitude;
      if ( grsafe <= 0 ) {
	/*
	 * We're under the safety altitude for this waypoint.
	 */
	break;
	//continue;
      }

      wpcalc.GR = wpcalc.Distance / grsafe;
      grsafe = wpcalc.GR;
      curgr = wpcalc.GR;

      if ( grsafe > grpolar ) {
	/*
	 * Over degraded polar GR for this waypoint
	 */
	continue;
	// break;
      }

      // Anything now is within degraded glide ratio, so if our homewaypoint is safely reachable then
      // attempt to lock it even if we already have a valid best, even if it is preferred and even
      // if it has a better GR

      if ( (SettingsComputer().HomeWaypoint >= 0) 
	   && (curwp == SettingsComputer().HomeWaypoint) ) {
	bestalternate = curwp;
	break;
      }

      // If we already found a preferred, stop searching for anything but home

      if (bestalternate >= 0 && way_points.get_calc(bestalternate).Preferred) {
	continue;
      }

      // VENTA5 TODO: extend search on other preferred, choosing the closer one

      // Preferred list has priority, first found is taken (could be smarted)

      if (wpcalc.Preferred) {
	bestalternate=curwp;
	continue;
      }

      // else we remember the best landable GR found so far. We shall use this in case
      // at the end of the search no home and no preferred were found.

      if ( curgr < curbestgr ) {
        if ((way_point.Flags & AIRPORT) == AIRPORT) {
	  curbestairport=curwp;
	  curbestgr=curgr; // ONLY FOR AIRPORT! NOT FOR OUTLANDINGS!!
	}
	else {
	  curbestoutlanding=curwp;
	}
      }

      continue;

    }

  if ( bestalternate <0 ) {

    if ( curbestairport >= 0 ) {
      bestalternate=curbestairport;
    } else {
      if ( curbestoutlanding >= 0 ) {
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
              if (way_points.get_calc(bestalternate).AltArrival < 0) {
		// Pick up the closest!
		if ( ValidWayPoint( SortedApproxIndex[0]) ) {
		  bestalternate=SortedApproxIndex[0];
		} else {
		  /// CRITIC POINT
		  // Otherwise .. nothing, either keep the old best or
		  // set it empty
		  // Put here "PULL-UP! PULL-UP! Boeing
		  // cockpit voice sound and possibly shake the stick.
		}
	      } else
		{
		  // MapWindow2 is checking for reachables separately,
		  // se let's see if this closest is reachable
		  if ( ValidWayPoint( SortedApproxIndex[0] )) {
                    if (way_points.get_calc(SortedApproxIndex[0]).Reachable) {
		      bestalternate = SortedApproxIndex[0];
		    } else
		      {
		      }
		  } else
		    {
		    }
		}
	    } else
	    {
	      /// CRITIC POINT
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
             ((safecalc - way_points.get(bestalternate).Altitude) > ALTERNATE_QUIETMARGIN)) {
          if (way_points.get_calc(bestalternate).AltArrivalAGL <100 )
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
    if ( SettingsComputer().HomeWaypoint >= 0 ) {
      bestalternate=SettingsComputer().HomeWaypoint;
    }
  } else {
    // If still invalid, i.e. not -1, then there's a big problem
    if ( !ValidWayPoint(bestalternate) ) {
      AlertBestAlternate(2);
      Message::AddMessage(_T("Error, invalid best alternate!"));
      // todo: immediate disable function
    }
  }

  if (active_bestalternate_on_entry != bestalternate) {
    SetCalculated().BestAlternate = bestalternate;
    if ( bestalternate >0 &&
         ((safecalc - way_points.get(bestalternate).Altitude) > ALTERNATE_QUIETMARGIN))
      AlertBestAlternate(1);
  }

  mutexTaskData.Unlock();
}

/*
 * Do not disturb too much. Play alert sound only once every x minutes, not more.
 */
void
GlideComputerTask::AlertBestAlternate(short soundmode)
{
  static double LastAlertTime=0;

  if ( Basic().Time > LastAlertTime + 180.0 ) {
    if (SettingsComputer().EnableSoundModes) {
      LastAlertTime = Basic().Time;
      switch (soundmode) {
      case 0:
	break;
      case 1:
	PlayResource(TEXT("IDR_WAV_GREEN"));
	break;
      case 2:
	PlayResource(TEXT("IDR_WAV_RED"));
	break;
      case 11:
	PlayResource(TEXT("IDR_WAV_GREEN"));
	PlayResource(TEXT("IDR_WAV_GREEN"));
	break;
      default:
	break;
      }
    }
  }
}

void
GlideComputerTask::DoBestAlternateSlow()
{
  static double LastSearchBestTime = 0; // VENTA3

 // VENTA3 best landing slow calculation
#ifdef WINDOWSPC
  if ( (SettingsComputer().EnableBestAlternate) && (Basic().Time > LastSearchBestTime+10.0) ) // VENTA3
#else
  if ( (SettingsComputer().EnableBestAlternate) && (Basic().Time > LastSearchBestTime+BESTALTERNATEINTERVAL) ) // VENTA3
#endif
    {
      LastSearchBestTime = Basic().Time;
      SearchBestAlternate();
    }

}


/*
 * VENTA3 Alternates destinations
 *
 * Used by Alternates and BestAlternate
 *
 * Colors VGR are disabled, but available
 */

void
GlideComputerTask::DoAlternates(int AltWaypoint)
{
  if (!ValidWayPoint(AltWaypoint)) {
    return;
  }

  const WAYPOINT &way_point = way_points.get(AltWaypoint);
  WPCALC &way_point_calc = way_points.set_calc(AltWaypoint);
  GEOPOINT w1 = way_point.Location;
  GEOPOINT w0 = Basic().Location;
  double *altwp_dist = &way_point_calc.Distance;
  double *altwp_gr = &way_point_calc.GR;
  double *altwp_arrival = &way_point_calc.AltArrival;
  short  *altwp_vgr = &way_point_calc.VGR;

  DistanceBearing(w1, w0, altwp_dist, NULL);

  double GRsafecalc = Calculated().NavAltitude - 
    (way_point.Altitude +
     SettingsComputer().SAFETYALTITUDEARRIVAL);

  if (GRsafecalc <=0) {
    *altwp_gr = INVALID_GR;
  } else {
    *altwp_gr = *altwp_dist / GRsafecalc;
    if ( *altwp_gr >ALTERNATE_MAXVALIDGR || *altwp_gr <0 ) {
      *altwp_gr = INVALID_GR;
    } else if ( *altwp_gr <1 ) {
      *altwp_gr = 1;
    }
  }


  // We need to calculate arrival also for BestAlternate, since the last "reachable" could be
  // even 60 seconds old and things may have changed drastically

  *altwp_arrival = CalculateWaypointArrivalAltitude(way_point, way_point_calc);
  if ( (*altwp_arrival - ALTERNATE_OVERSAFETY) >0 ) {
    if ( *altwp_gr <= (GlidePolar::bestld *SAFELD_FACTOR) ) {
      *altwp_vgr = 1; // full green vgr
    } else if ( *altwp_gr <= GlidePolar::bestld ) {
      *altwp_vgr = 2; // yellow vgr
    } else *altwp_vgr =3; // RED vgr
  } else {
    *altwp_vgr = 3; // full red
  }
}
