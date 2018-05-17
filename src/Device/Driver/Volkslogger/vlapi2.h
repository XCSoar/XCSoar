/***********************************************************************
 **
 **   vlapi2.h
 **
 **   This file is part of libkfrgcs.
 **
 ************************************************************************
 **
 **   Copyright (c):  2002 by Garrecht Ingenieurgesellschaft
 **
 **   This file is distributed under the terms of the General Public
 **   Licence. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

#ifndef VLAPI2_H
#define VLAPI2_H

#include "Geo/GeoPoint.hpp"

#include <stdint.h>

class DBB;

class VLAPI_DATA {
 public:
  // waypoint
  struct WPT {
    // waypoint attribute(type) flags
    enum WPTTYP {
      // landable -> WPT will be included in nearest wapoint search (NAV/EMR)
      WPTTYP_L = 1,
      // hard-surfaced landing site -> additional information about WPT
      WPTTYP_H = 2,
      // airport -> additional information about WPT
      WPTTYP_A = 4,
      // checkpoint -> WPT will not change any sector direction etc.
      WPTTYP_C = 8
    };
    char name[7]; // name of waypoint, zero-ended C-string, max. 6 characters
    GeoPoint location;
    uint8_t typ; // type(attributes) of WPT, or-combination of enum WPTTYP

    void get(const void *p);
    void put(void *p) const;
  };

  // declaration-waypoint
  // this is the database waypoint extended by information about the
  // observation zone for each point of the flight-declaration (task)
  struct DCLWPT : public WPT {
    // either can the observation zone be a cyl/sector combination
    // or can it be a line, but it cannot be both
    // oztype determines which variables(rz/rs or lw) are used
    enum OZTYP {
      OZTYP_CYLSKT = 0,
      OZTYP_LINE = 1
    };
    unsigned lw; // linewidth (start- or finishline) in kilometers
    unsigned rz;  // cylinder radius in meters (0..1500m)
    // the API will round this to the next lower 100m-step
    unsigned rs;  // sector radius in meters   (0..15000m)
    // the API will round this to the next lower 1000m-step
    unsigned ws;  // sector direction in degrees
    // 0..358 directly specifies the direction
    // into which the 90 degree FAI-sector points
    // 360 means automatic calculation of the direction inside
    // the VL, according to FAI-rules
    OZTYP oztyp;

    void get(const void *p);
    void put(void *p) const;
  };

  struct ROUTE {
    char name[15];
    WPT wpt[10];

    void get(const void *p);
    void put(void *p) const;
  };

  struct PILOT {
    char name[17];

    void get(const void *p);
    void put(void *p) const;
  };

  struct DATABASE {
    int nwpts;
    WPT *wpts;
    int nroutes;
    ROUTE *routes;
    int npilots;
    PILOT *pilots;

    void CopyFrom(const DBB &dbb);
    void CopyTo(DBB &dbb) const;
  };

  // flight declaration
  struct DECLARATION {
    struct FLIGHTINFO {
      char pilot[65];
      char gliderid[8];
      char glidertype[13];
      char competitionclass[13];
      char competitionid[4];
      WPT homepoint;
    } flightinfo;
    struct TASK {
      DCLWPT startpoint;
      DCLWPT finishpoint;
      unsigned nturnpoints;
      DCLWPT turnpoints[12];
    } task;

    void get(const DBB &dbb);
    void put(DBB *dbb) const;
  };
};

#endif
