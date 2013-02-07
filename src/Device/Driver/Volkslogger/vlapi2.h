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

#include "vlapierr.h"
#include "vla_support.h"
#include "vlconv.h"
#include "Geo/GeoPoint.hpp"
#include "tchar.h"

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
    int16  lw; // linewidth (start- or finishline)
    int16  rz;  // cylinder radius in meters (0..1500m)
    // the API will round this to the next lower 100m-step
    int16  rs;  // sector radius in meters   (0..15000m)
    // the API will round this to the next lower 1000m-step
    int16  ws;  // sector direction in degrees
    // 0..358ø directly specifies the direction
    // into which the 90°-FAI-sector points
    // 360ø means automatic calculation of the direction inside
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
  class DECLARATION {
    friend class VLAPI;
  public:
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
      int nturnpoints;
      DCLWPT turnpoints[12];
    } task;
  protected:
    void get(DBB *dbb);
    void put(DBB *dbb) const;
  };

  struct VLINFO {
    word sessionid;
    word vlserno;
    uint8_t fwmajor;
    uint8_t fwminor;
    uint8_t fwbuild;
  };
};


// just instatiate an Object of VLAPI in your application
// and call the functions
// all data exchange with the API will be done through its
// public members
/** API facade for Volkslogger device handler */
class VLAPI : public VLA_XFR, public VLAPI_DATA {
  VLA_ERROR stillconnect();
 public:

  VLAPI(Port &_port, unsigned _databaud, OperationEnvironment &env);

  VLINFO vlinfo;
  DATABASE database;
  DECLARATION declaration;
  DIRECTORY directory; // struct DIRECTORY declared in VLCONV.H

  // read info (serial numer, firmware versions etc.) from
  // the logger into the struct VLINFO (see above)
  VLA_ERROR read_info();

  // returns version of this API
  int apiversion() {
    return 200;
  }

  // read the directory of flight logs into struct DIRECTORY (see file VLCONV.H)
  VLA_ERROR read_directory();

  // read igcfile number index (position in array contained in struct DIRECTORY )
  // into file named "filename".
  // secure = 1 for DSA-signature, 0 for MD-signature only
  // DSA is mandatory for DMST and FAI flight validation
  VLA_ERROR read_igcfile(const TCHAR *filename, int index, int secure);

  // read database and flight declaration form from Volkslogger into the
  // predefined structs DECLARATION and DATABASE (see above)
  VLA_ERROR read_db_and_declaration();

  // write database and flight declaration from the structs back into the Volkslogger
  VLA_ERROR write_db_and_declaration();
};

#endif
