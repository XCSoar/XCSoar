/***********************************************************************
 **
 **   vlapi2.cpp
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

#include "vlapi2.h"
#include "dbbconv.h"
#include "grecord.h"
#include "utils.h"
#include "Database.hpp"
#include "Device/Port/Port.hpp"
#include "Util.hpp"
#include "Protocol.hpp"
#include "Operation/Operation.hpp"
#include "Util/CharUtil.hpp"

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

// sizes of VL memory regions
const int32 VLAPI_LOG_MEMSIZE = 81920L;

// ------------------------------------------------------------
//                        VLA_XFR
// ------------------------------------------------------------

VLA_XFR::VLA_XFR(Port &_port, unsigned _databaud, OperationEnvironment &_env)
  :port(_port), env(_env), databaud(_databaud) {
}

// ------------------------------------------------------------
//                          VLAPI
// ------------------------------------------------------------

// constructor
VLAPI::VLAPI(Port &_port, unsigned _databaud, OperationEnvironment &_env)
  :VLA_XFR(_port, _databaud, _env) {
}

VLA_ERROR
VLAPI::read_info(VLINFO &vlinfo)
{
  uint8_t buffer[8];

  if (Volkslogger::ReadInfo(port, env,
                            buffer, sizeof(buffer)) < (int)sizeof(buffer))
    return VLA_ERR_NODATA;

  // Aufbau der Versions- und sonstigen Nummern
  vlinfo.sessionid = 256*buffer[0] + buffer[1];
  vlinfo.vlserno = 256*buffer[2] + buffer[3];
  vlinfo.fwmajor = buffer[4] / 16;
  vlinfo.fwminor = buffer[4] % 16;
  vlinfo.fwbuild = buffer[7];

  return VLA_ERR_NOERR;
}

// tries a connect without user interaction first.
// if this fails, ask user to connect the VOLKSLOGGER
VLA_ERROR VLAPI::stillconnect() {
  return Volkslogger::ConnectAndFlush(port, env, 4000) ||
    Volkslogger::ConnectAndFlush(port, env, 10000)
    ? VLA_ERR_NOERR
    : VLA_ERR_NODATA;
}

VLA_ERROR
VLAPI::read_db_and_declaration(DATABASE &database, DECLARATION &declaration)
{
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  DBB dbb1;
  if (Volkslogger::ReadDatabase(port, databaud, env,
                                dbb1.buffer, sizeof(dbb1.buffer)) <= 0)
    return VLA_ERR_NODATA;

  dbb1.open_dbb();

  database.CopyFrom(dbb1);
  declaration.get(&dbb1);

  return VLA_ERR_NOERR;
}


VLA_ERROR
VLAPI::write_db_and_declaration(const DATABASE &database,
                                const DECLARATION &declaration)
{
  DBB dbb1;
  database.CopyTo(dbb1);
  declaration.put(&dbb1);

  // and write buffer back into VOLKSLOGGER
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  if (!Volkslogger::WriteDatabase(port, env, dbb1.buffer, sizeof(dbb1.buffer)))
      return VLA_ERR_NODATA;

  return VLA_ERR_NOERR;
}

VLA_ERROR
VLAPI::update_logger_declaration(const DECLARATION &declaration)
{
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  //populate DBB structure with database(=block) read from logger
  DBB dbb1;
  if (Volkslogger::ReadDatabase(port, databaud, env,
                                dbb1.buffer, sizeof(dbb1.buffer)) <= 0)
    return VLA_ERR_NODATA;

  //do NOT use the declaration(=fdf) from logger
  memset(dbb1.GetFDF(), 0xff, dbb1.FRM_SIZE);

  dbb1.open_dbb();

  //update declaration section
  declaration.put(&dbb1);

  // and write buffer back into VOLKSLOGGER
  err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  if (!Volkslogger::WriteDatabase(port, env, dbb1.buffer, sizeof(dbb1.buffer)))
      return VLA_ERR_NODATA;

  return VLA_ERR_NOERR;
}

VLA_ERROR
VLAPI::read_directory(std::vector<DIRENTRY> &directory)
{
  directory.reserve(10);
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  uint8_t dirbuffer[VLAPI_LOG_MEMSIZE];
  int data_length = Volkslogger::ReadFlightList(port, env,
                                                dirbuffer, sizeof(dirbuffer));

  if (data_length == -1)
    return VLA_ERR_MISC;

  if(data_length > 0) {
    if (!conv_dir(directory, dirbuffer, data_length)) {
      directory.clear();
      return VLA_ERR_MISC;
    }

    if(!directory.empty())
      return VLA_ERR_NOERR;
    else
      return VLA_ERR_NOFLIGHTS;
  }
  else {
    directory.clear();
    return VLA_ERR_NOFLIGHTS;
  }

  return VLA_ERR_MISC;

}

VLA_ERROR
VLAPI::read_igcfile(const TCHAR *filename, unsigned index, bool secmode)
{
  VLA_ERROR err = stillconnect();
  if (err != VLA_ERR_NOERR)
    return err;

  uint8_t logbuffer[VLAPI_LOG_MEMSIZE];
  const size_t length = Volkslogger::ReadFlight(port, databaud, env,
                                                index, secmode,
                                                logbuffer, sizeof(logbuffer));
  if (length == 0)
    return VLA_ERR_MISC;

  FILE *outfile = _tfopen(filename, _T("wt"));
  if (outfile == nullptr)
    return VLA_ERR_FILE;

  size_t r = convert_gcs(0, outfile, logbuffer, length, true);
  if (r > 0) {
    err = VLA_ERR_NOERR;
    print_g_record(outfile,   // output to stdout
                   logbuffer, // binary file is in buffer
                   r          // length of binary file to include
                   );
  } else
    err = VLA_ERR_MISC;

  fclose(outfile);
  return err;
}


// getting a waypoint object out of the database memory
//
void VLAPI_DATA::WPT::get(const void *p) {
  const Volkslogger::Waypoint *src = (const Volkslogger::Waypoint *)p;

  CopyTerminatedUpper(name, src->name, sizeof(src->name));

  typ = (WPTTYP)(src->type_and_longitude_sign & 0x7f);

  location = src->GetLocation();
}



// putting a waypoint object into the database memory
//
void VLAPI_DATA::WPT::put(void *p) const {
  Volkslogger::Waypoint *dest = (Volkslogger::Waypoint *)p;
  // String, evtl. mit Blanks aufgef�llt, zur�ckschreiben
  CopyPaddedUpper(dest->name, sizeof(dest->name), name);

  dest->type_and_longitude_sign = (typ & 0x7f);
  dest->SetLocation(location);
}

// getting a declaration waypoint object out of the database
//
void VLAPI_DATA::DCLWPT::get(const void *p) {
  const Volkslogger::DeclarationWaypoint *src =
    (const Volkslogger::DeclarationWaypoint *)p;

  WPT::get(src);

  oztyp = (OZTYP)src->oz_shape;
  ws = src->direction * 2;
  if(oztyp == OZTYP_LINE) {
    lw = (src->oz_parameter & 0x0f) * ((src->oz_parameter & 0xf0) >> 4);
  }
  else {
    rz = (src->oz_parameter & 0x0f) * 100;
    rs = ((src->oz_parameter & 0xf0) >> 4) * 1000;
  }
}

// putting a declaration waypoint object into the database memory
//
void VLAPI_DATA::DCLWPT::put(void *p) const {
  Volkslogger::DeclarationWaypoint *dest =
    (Volkslogger::DeclarationWaypoint *)p;

  WPT::put(dest);

  dest->oz_shape = oztyp;
  dest->direction = ws / 2;

  if(oztyp == OZTYP_LINE) {
    // find two integer numbers between 1 and 15 the product of which
    // is just lower or equal the linewidth
    int w1 = 0;
    int w2 = 0;
    for(int i=1;i<=15;i++) {
      if(lw%i==0 && lw/i <= 15) {
        w1 = i;
        w2 = lw/i;
        break;
      }
    }

    dest->oz_parameter = (w1 << 4) + w2;
  }
  else {
    dest->oz_parameter = (rz / 100) + ((rs / 1000 ) << 4);
  }
}

void VLAPI_DATA::ROUTE::get(const void *p) {
  const Volkslogger::Route *src = (const Volkslogger::Route *)p;

  CopyTerminatedUpper(name, src->name, sizeof(src->name));

  for(int i=0; i<10; i++)
    wpt[i].get(&src->waypoints[i]);
}

void VLAPI_DATA::ROUTE::put(void *p) const {
  Volkslogger::Route *dest = (Volkslogger::Route *)p;

	int i;
  CopyPaddedUpper(dest->name, sizeof(dest->name), name);
  // In the following line, we insertes "int"
  // (Florian Ehinger)
  for(i=0; i<10; i++)
    wpt[i].put(&dest->waypoints[i]);
}

void
VLAPI_DATA::PILOT::get(const void *p)
{
  const Volkslogger::Pilot *src = (const Volkslogger::Pilot *)p;

  CopyTerminatedUpper(name, src->name, sizeof(src->name));
}

void
VLAPI_DATA::PILOT::put(void * p) const
{
  Volkslogger::Pilot *dest = (Volkslogger::Pilot *)p;

  CopyPaddedUpper(dest->name, sizeof(dest->name), name);
}

void
VLAPI_DATA::DATABASE::CopyFrom(const DBB &dbb)
{
  // convert and write list of waypoints
  if (dbb.header[0].dsfirst != 0xffff) {
    nwpts = 1 + (dbb.header[0].dslast - dbb.header[0].dsfirst)
      / dbb.header[0].dslaenge;
    delete[] wpts;
    wpts = new WPT[nwpts];
    for (int i=0; i<nwpts; i++) {
      wpts[i].get(dbb.GetBlock(dbb.header[0].dsfirst +
                               i * dbb.header[0].dslaenge));
    }
  }

  // convert and write list of routes
  if (dbb.header[3].dsfirst != 0xffff) {
    nroutes = 1 + (dbb.header[3].dslast - dbb.header[3].dsfirst)
      / dbb.header[3].dslaenge ;
    delete[] routes;
    routes = new ROUTE[nroutes];
    for (int i=0; i<nroutes; i++) {
      routes[i].get(dbb.GetBlock(dbb.header[3].dsfirst +
                                 i * dbb.header[3].dslaenge));
    }
  }

  // convert and write list of pilots
  if (dbb.header[1].dsfirst != 0xffff) {
    npilots = 1 + (dbb.header[1].dslast - dbb.header[1].dsfirst)
      / dbb.header[1].dslaenge;
    delete[] pilots;
    pilots = new PILOT[npilots];
    for (int i=0; i<npilots; i++) {
      pilots[i].get(dbb.GetBlock(dbb.header[1].dsfirst +
                                 i * dbb.header[1].dslaenge));
    }
  }
}

void
VLAPI_DATA::DATABASE::CopyTo(DBB &dbb) const
{
  dbb.open_dbb();

  for (int i = 0; i < nwpts; ++i) {
    uint8_t bwpt[13];
    wpts[i].put(bwpt);
    dbb.add_ds(0, bwpt);
  }
  dbb.close_db(0);

  for (int i = 0; i < npilots; ++i) {
    uint8_t bpilot[17];
    pilots[i].put(bpilot);
    dbb.add_ds(1, bpilot);
  }
  dbb.close_db(1);

  for (int i = 0; i < nroutes; ++i) {
    uint8_t broute[144];
    routes[i].put(broute);
    dbb.add_ds(3, broute);
  }
  dbb.close_db(3);
}

// read flight-declaration fields database into structure
//
void VLAPI_DATA::DECLARATION::get(DBB *dbb) {
  int p;
  char plt1[17];
  char plt2[17];
  char plt3[17];
  char plt4[17];
  plt1[0] = 0;
  plt2[0] = 0;
  plt3[0] = 0;
  plt4[0] = 0;
  if ((p = dbb->fdf_findfield(FLDPLT1))>=0)
    strncpy(plt1, (const char *)dbb->GetFDF(p + 2), sizeof(plt1));
  if ((p = dbb->fdf_findfield(FLDPLT2))>=0)
    strncpy(plt2, (const char *)dbb->GetFDF(p + 2), sizeof(plt2));
  if ((p = dbb->fdf_findfield(FLDPLT3))>=0)
    strncpy(plt3, (const char *)dbb->GetFDF(p + 2), sizeof(plt3));
  if ((p = dbb->fdf_findfield(FLDPLT4))>=0)
    strncpy(plt4, (const char *)dbb->GetFDF(p + 2), sizeof(plt4));
  flightinfo.pilot[0] = 0;
  strcat(flightinfo.pilot,plt1);
  strcat(flightinfo.pilot,plt2);
  strcat(flightinfo.pilot,plt3);
  strcat(flightinfo.pilot,plt4);
  if ((p = dbb->fdf_findfield(FLDGTY))>=0)
    strncpy(flightinfo.glidertype, (const char *)dbb->GetFDF(p + 2),
            sizeof(flightinfo.glidertype));
  if ((p = dbb->fdf_findfield(FLDGID))>=0)
    strncpy(flightinfo.gliderid, (const char *)dbb->GetFDF(p + 2),
            sizeof(flightinfo.gliderid));
  if ((p = dbb->fdf_findfield(FLDCCL))>=0)
    strncpy(flightinfo.competitionclass, (const char *)dbb->GetFDF(p + 2),
            sizeof(flightinfo.competitionclass));
  if ((p = dbb->fdf_findfield(FLDCID))>=0)
    strncpy(flightinfo.competitionid, (const char *)dbb->GetFDF(p + 2),
            sizeof(flightinfo.competitionid));
  if ((p = dbb->fdf_findfield(FLDTKF))>=0)
    flightinfo.homepoint.get(dbb->GetFDF(p + 2));

  if ((p = dbb->fdf_findfield(FLDSTA))>=0)
    task.startpoint.get(dbb->GetFDF(p + 2));
  if ((p = dbb->fdf_findfield(FLDFIN))>=0)
    task.finishpoint.get(dbb->GetFDF(p + 2));
  if ((p = dbb->fdf_findfield(FLDNTP))>=0)
    task.nturnpoints = *(const uint8_t *)dbb->GetFDF(p + 2);
  for (unsigned i = 0; i < task.nturnpoints; ++i) {
    if ((p = dbb->fdf_findfield(FLDTP1+i))>=0)
      task.turnpoints[i].get(dbb->GetFDF(p + 2));
  }
}

void
VLAPI_DATA::DECLARATION::put(DBB *dbb) const
{
  const char *src = flightinfo.pilot;

  for (unsigned i = 0; i < 4; ++i) {
    char *dest = (char *)dbb->AddFDF(FLDPLT + i, 17);
    if (dest == nullptr)
      break;

    for (unsigned i = 0; i < 16; ++i)
      *dest++ = ToUpperASCII(*src++);
    *dest = '\0';
  }

  dbb->AddFDFStringUpper(FLDGTY, flightinfo.glidertype);
  dbb->AddFDFStringUpper(FLDGID, flightinfo.gliderid);
  dbb->AddFDFStringUpper(FLDCCL, flightinfo.competitionclass);
  dbb->AddFDFStringUpper(FLDCID, flightinfo.competitionid);

  uint8_t fdfwpt[16]; // temporary space for data conversions
  flightinfo.homepoint.put(fdfwpt);
  dbb->add_fdf(FLDTKF,sizeof(fdfwpt),&fdfwpt);

  uint8_t ntp = task.nturnpoints;
  dbb->add_fdf(FLDNTP,1,&ntp);

  task.startpoint.put(fdfwpt);
  dbb->add_fdf(FLDSTA,sizeof(fdfwpt),&fdfwpt);

  task.finishpoint.put(fdfwpt);
  dbb->add_fdf(FLDFIN,sizeof(fdfwpt),&fdfwpt);

  for (unsigned i = 0; i < task.nturnpoints; ++i) {
    task.turnpoints[i].put(fdfwpt);
    dbb->add_fdf(FLDTP1+i,sizeof(fdfwpt),&fdfwpt);
  }
}
