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

#include "Device/Volkslogger/vlapi2.h"
#include "Device/Volkslogger/dbbconv.h"
#include "Device/Volkslogger/grecord.h"
#include "Device/Volkslogger/utils.h"
#include "Device/Port.hpp"
#include "Util.hpp"
#include "CRC16.hpp"
#include "PeriodClock.hpp"
#include "Operation.hpp"

#include <memory.h>
#include <string.h>
#include <stdlib.h>

extern int noninteractive;

// sizes of VL memory regions
const int VLAPI_DBB_MEMSIZE = 16384;
const int32 VLAPI_LOG_MEMSIZE = 81920L;

void
VLAPI::set_port(Port *_port)
{
  port = _port;
}

// ------------------------------------------------------------
//                        VLA_XFR
// ------------------------------------------------------------

int32 VLA_XFR::commandbaud = 9600L;

// set baudrate
//
void VLA_XFR::set_databaud(int32 db) {
  databaud = db;
  switch(db) {
  case   9600 :
    databaudidx = 1;
    break;
  case  19200 :
    databaudidx = 2;
    break;
  case  38400L :
    databaudidx = 3;
    break;
  case  57600L :
    databaudidx = 4;
    break;
  case 115200L :
    databaudidx = 5;
    break;
  default     :
    databaudidx = 1;
    break;
  }
}

bool
VLA_XFR::SendWithCRC(const void *data, size_t length)
{
  if (!port->FullWrite(data, length, 2000))
    return false;

  uint16_t crc16 = UpdateCRC(data, length, 0);
  serial_out(crc16 >> 8);
  serial_out(crc16 & 0xff);

  return true;
}

// send command to VOLKSLOGGER
//
int16 VLA_XFR::sendcommand(byte cmd, byte param1, byte param2) {
  byte	        c=255;
  const int16   d = 2;  //Verzögerungszeit 2ms
  byte 		cmdarray[8];
  // alte Zeichen verwerfen
  env.Sleep(100);
  serial_empty_io_buffers();

  // Kommandointerpreter im VL zurücksetzen
  for (unsigned i =0 ; i < 6; i++) {
    serial_out(CAN);
    env.Sleep(d);
  }
  // Kommandopaket aufbauen

  cmdarray[0] = cmd;
  cmdarray[1] = param1;
  cmdarray[2] = param2;
  cmdarray[3] = 0;
  cmdarray[4] = 0;
  cmdarray[5] = 0;
  cmdarray[6] = 0;
  cmdarray[7] = 0;
  // Kommando verschicken ( ENQ,Daten,CRC )
  serial_out(ENQ);
  env.Sleep(d);

  if (!SendWithCRC(cmdarray, sizeof(cmdarray))) {
    showwait(VLS_TXT_NOFR);
    return -1;
  }

  // Kommandobestätigung abwarten, aber höchstens timeout Sekunden
  const unsigned timeout_ms = 4000;
  PeriodClock clock;
  clock.update();
  while (serial_in(&c) != VLA_ERR_NOERR) {
    if (clock.check(timeout_ms)) {
      showwait(VLS_TXT_NOFR);
      return -1;
    }

    progress_set(VLS_TXT_SENDCMD);
  }

  // Fehler (timeout oder Fehlercode vom VL) im Klartext ausgeben
  switch (c) {
  case 0:
    show(VLS_TXT_NIL);
    break;
  case 1:
    showwait(VLS_TXT_BADCMD);
    break;
  case 2:
    showwait(VLS_TXT_WRONGFR);
    break;
  }
  // Fehlercode als Rückgabewert der Funktion benutzen
  return c;
  // Rückgabewert:     -1 : timeout
  //		 0..255 : Bestätigungscode vom Logger
}


// wait until completion of command in the VL
//
int16 VLA_XFR::wait4ack() {
  byte	c;
  // Anfangszeit merken
  const unsigned timeout_ms = 180000;
  PeriodClock clock;
  clock.update();
  // Auf Beendigungscode vom Logger warten
  while (serial_in(&c) != VLA_ERR_NOERR) {
    if (test_user_break() && clear_user_break() == 1)
      return 255;

    if (clock.check(timeout_ms))
      return 255;

    progress_set(VLS_TXT_WTCMD);
  }

  return c;
}


// read a big data packet from the VL after requesting it with a command
// via sendcommand. Read a maximum of maxlen characters (excluding CRC)
//
int32 VLA_XFR::readlog(lpb puffer, int32 maxlen) {
  int32 gcs_counter = 0;
  byte  c;
  int16 dle_r = 0;
  word crc16 = 0;
  int16 start = 0;
  int16 ende  = 0;
//  int32 i;
  lpb p;
  int pp = 0;
  progress_reset();

  memset(puffer, 0xff, maxlen);

  p = puffer;
  env.Sleep(300);
  while(!ende) {
    // Zeichen anfordern und darauf warten
    serial_out(ACK);

    pp++;
    serial_in(&c);
    if ((gcs_counter>=maxlen) && (crc16==0)) {
      ende = 1;
    }

    // dabei ist Benutzerabbruch jederzeit möglich
    if (test_user_break() && clear_user_break() == 1) {
      env.Sleep(10);
      serial_out(CAN);
      serial_out(CAN);
      serial_out(CAN);

      show(VLS_TXT_UIRQ);
      gcs_counter = 0;
      return -1;
    }

    // oder aber das empfangene Zeichen wird ausgewertet
    switch (c) {
    case DLE:
      if (dle_r == 0) {             //!DLE, DLE -> Achtung!
        dle_r = 1;
      }
      else { 	                 // DLE, DLE -> DLE-Zeichen
        dle_r = 0;
        if (start) {
          if(gcs_counter < maxlen)
            *p++ = c;
          gcs_counter++;
          crc16 = UpdateCRC(c,crc16);
        }
      }
      break;
    case ETX:
      if (dle_r == 0) {             //!DLE, ETX -> Zeichen
        if (start) {
          if(gcs_counter < maxlen) {
            *p++ = c;
          }
          gcs_counter++;
          crc16 = UpdateCRC(c,crc16);
        };
      }
      else {
        if (start==1) {
          ende = 1;                   // DLE, ETX -> Blockende
          dle_r = 0;
        }
      }
      break;
    case STX:
      if (dle_r == 0) {	         //!DLE, STX -> Zeichen
        if (start) {
          if(gcs_counter < maxlen)
            *p++ = c;
          gcs_counter++;
          crc16 = UpdateCRC(c,crc16);
        }
      }
      else {
        start = 1;           // DLE, STX -> Blockstart
        dle_r = 0;
        crc16 = 0;
        progress_set(VLS_TXT_XFERRING);
      }
      break;
    default:
      if (start) {
        if(gcs_counter < maxlen)
          *p++ = c;
        gcs_counter++;
        crc16 = UpdateCRC(c,crc16);
      }
      break;
    }

    if (gcs_counter == maxlen) {
      //    ende = 1; // JMW cheat quick exit... TODO fix me
    }

  }
  env.Sleep(100);

  if (crc16) {
    show(VLS_TXT_CRC);
    return -1;
  }

  if (gcs_counter < 2) {
    show(VLS_TXT_EMPTY);
    return 0;
  }

  // CRC am Ende abschneiden
  return gcs_counter - 2;
}


// Blockweises Schreiben in den Logger (z.B. Datenbank)
VLA_ERROR VLA_XFR::dbbput(lpb dbbbuffer, int32 dbbsize) {
  word crc16;
  byte c;
  int32 td = 1;

  // Schreibkommando geben
  serial_empty_io_buffers();
  sendcommand(cmd_PDB,0,0); // muß noch mit Timeout versehen werden
  // auf Löschende warten
  while (serial_in(&c) != VLA_ERR_NOERR) {
    if (test_user_break() && clear_user_break() == 1) {
      showwait(VLS_TXT_UIRQ2);
      return VLA_ERR_USERCANCELED;
    }
  }

  // Fehlerbehandlung
  if (c != ACK)
    return VLA_ERR_MISC;
  // Schreiben der Datenbank
  env.Sleep(100);
  crc16 = 0;

  const uint8_t *p = (const uint8_t *)dbbbuffer, *end = p + dbbsize;
  while (p < end) {
    size_t n = end - p;
    if (n > 400)
      n = 400;

    n = port->Write(p, n);
    if (n == 0)
      return VLA_ERR_MISC;

    crc16 = UpdateCRC(p, n, crc16);
    p += n;

    progress_set(VLS_TXT_WDB);

    /* throttle sending a bit, or the Volkslogger's receive buffer
       will overrun */
    env.Sleep(td * 100);
  }

  serial_out(crc16/256);
  env.Sleep(td);
  serial_out(crc16%256);
  env.Sleep(td);
  // auf Bestätigung warten
  while (serial_in(&c) != VLA_ERR_NOERR) {
    if (test_user_break() && clear_user_break() == 1) {
      showwait(VLS_TXT_UIRQ2);
      return VLA_ERR_USERCANCELED;
    }
  }

  // Fehlerbehandlung
  if (c != ACK)
    return VLA_ERR_MISC;
  return VLA_ERR_NOERR;
}


VLA_ERROR VLA_XFR::dbbget(lpb dbbbuffer, int32 dbbsize) {
  int32 groesse;
  // Datenbanklesekommando abschicken
  if (sendcommand(cmd_RDB,0,databaudidx))
    return VLA_ERR_BADCOMMAND;
  // DATA-Baudrate einstellen, Datenbank lesen, CMD-Baudrate einstellen
  serial_set_baudrate(databaud);
  groesse = readlog(dbbbuffer,dbbsize);
  serial_set_baudrate(commandbaud);
  env.Sleep(300);
  if (groesse <= 0)
    return VLA_ERR_NODATA;
  // und Tschüß
  return VLA_ERR_NOERR;
}

VLA_ERROR VLA_XFR::readdir(lpb buffer, int32 size) {
  int r;
  if(buffer==0)
    return VLA_ERR_MISC;
  if(sendcommand(cmd_DIR,0,0) != 0)
    return VLA_ERR_NOANSWER;
  r = readlog(buffer,size);
  if (r <= 0)
    return VLA_ERR_NOFLIGHTS;
  else
    return VLA_ERR_NOERR;
}

VLA_ERROR VLA_XFR::all_logsget(lpb dbbbuffer, int32 dbbsize) {
  int32 groesse;
  // Datenbanklesekommando abschicken
  if (sendcommand(cmd_ERO,0,databaudidx))
    return VLA_ERR_BADCOMMAND;
  // DATA-Baudrate einstellen, log-memory lesen, CMD-Baudrate einstellen
  serial_set_baudrate(databaud);
  groesse = readlog(dbbbuffer,dbbsize);
  serial_set_baudrate(commandbaud);
  env.Sleep(300);
  if (groesse <= 0)
    return VLA_ERR_NODATA;
  // und Tschüß
  return VLA_ERR_NOERR;
}


/*
  Auslesen des Fluges flightnumber im Sicherheitslevel secmode,
  Abspeichern als GCS-Datei im Speicher
*/
long VLA_XFR::flightget(lpb buffer, int32 buffersize, int16 flightnr, int16 secmode) {
  long groesse = 0;
  long sgr = 0;

  int cret;

  // read binary flightlog
  if (secmode)
    cret = sendcommand(cmd_GFS, flightnr,databaudidx);
  else
    cret = sendcommand(cmd_GFL, flightnr,databaudidx);

  if (cret)
    return 0;

  serial_set_baudrate(databaud); // DATA-Baudrate einstellen

  groesse = readlog(buffer,buffersize);

  if (groesse <= 0)
    return 0;

  // read signature
  serial_set_baudrate(commandbaud); // CMD-Baudrate einstellen
  env.Sleep(300);
  cret = sendcommand(cmd_SIG, 0,0);

  if (cret)
    return 0;

  if ((sgr = readlog(buffer+groesse,buffersize-groesse))<=0)
    return 0;

  return groesse + sgr;
}


//
// wait a specified amount of seconds (waittime) for connection of the
// VL to the serial port
//
VLA_ERROR VLA_XFR::connect(int32 waittime, int quietmode ) {
  int16 l_count = 0;
  int16 i;
  byte c;

  if(!quietmode)
    show(VLS_TXT_CONNECT);

  serial_empty_io_buffers();
  // eventuell noch laufende Aktion im Logger abbrechen
  for (i=0; i<10; i++) {
    serial_out(CAN);
    env.Sleep(1);
  }
  c = 0;

  const unsigned timeout_ms = waittime * 1000;
  PeriodClock clock;
  clock.update();

  do { // Solange R's aussenden, bis ein L zurückkommt
    serial_out('R');
    env.Sleep(30);

    if (clock.check(timeout_ms)) {
      if (!quietmode)
        show(VLS_TXT_CONN_FL);
      return VLA_ERR_NOANSWER;
    }
  } while (serial_in(&c) != VLA_ERR_NOERR || c != 'L');

  l_count = 1;
  while (true) { // Auf 4 hintereinanderfolgende L's warten
    if (serial_in(&c) == VLA_ERR_NOERR) {
      if (c != 'L') {
        if (!quietmode)
          show(VLS_TXT_CONN_FL);
        return VLA_ERR_NOANSWER;
      }

      l_count++;
      if (l_count >= 4)
        break;
    }

    if (clock.check(timeout_ms)) {
      if (!quietmode)
        show(VLS_TXT_CONN_FL);
      return VLA_ERR_TIMEOUT;
    }
  }

  if (!quietmode)
    show(VLS_TXT_CONN_OK);

  env.Sleep(300);
  serial_empty_io_buffers();
  return VLA_ERR_NOERR;
}

VLA_XFR::VLA_XFR(OperationEnvironment &_env):env(_env) {
  set_databaud(9600L);
}


VLA_ERROR VLA_XFR::readinfo(lpb buffer, int32 buffersize) {

  if(sendcommand(cmd_INF,0,0) != 0)
    return VLA_ERR_MISC;

  if((readlog(buffer, buffersize)) <= 0)
    return VLA_ERR_NODATA;

  return VLA_ERR_NOERR;
}


// ------------------------------------------------------------
//                          VLAPI
// ------------------------------------------------------------

// constructor
VLAPI::VLAPI(OperationEnvironment &_env):VLA_XFR(_env) {


  vlpresent = 0;
}

VLAPI::~VLAPI() {
  close(1);
}

// open a connection to the VL
//
// return values;
// -1 : port not accessible
// 0  : logger not present
// >0 : logger present
//
VLA_ERROR VLAPI::open(boolean connectit, int timeout,
                      boolean quiet, int32 sbaudrate) {
  noninteractive = quiet;
  VLA_ERROR err;

  // setup port
  if ((err = serial_set_baudrate(commandbaud)) != VLA_ERR_NOERR)
    return err;

  set_databaud(sbaudrate);
  // connect
  if (connectit) {
    if ((err = connect(timeout,quiet)) != VLA_ERR_NOERR)
      return err;

    vlpresent = 1;
    // nach erfolgreichem Connect
    // noninteractive wieder auf "Still" setzen
    //noninteractive = 1;
  }

  return err;
}



// close connection to VL
void VLAPI::close(boolean reset) {
  if(vlpresent) {
    if(reset) {
      sendcommand(cmd_RST,0,0);
    }
    vlpresent = 0;
  }
}


VLA_ERROR VLAPI::read_info() {
  byte buffer[8];
  VLA_ERROR err;

  if ((err = readinfo(buffer, sizeof(buffer))) == VLA_ERR_NOERR) {
    // Aufbau der Versions- und sonstigen Nummern
    vlinfo.sessionid = 256*buffer[0] + buffer[1];
    vlinfo.vlserno = 256*buffer[2] + buffer[3];
    vlinfo.fwmajor = buffer[4] / 16;
    vlinfo.fwminor = buffer[4] % 16;
    vlinfo.fwbuild = buffer[7];
  }

  return err;
}

// tries a connect without user interaction first.
// if this fails, ask user to connect the VOLKSLOGGER
VLA_ERROR VLAPI::stillconnect() {
  VLA_ERROR err;
  err = connect(4,1);
  if(err != VLA_ERR_NOERR)
    err = connect(10,0);
  if (err == VLA_ERR_NOERR)
    vlpresent = 1;
  return err;
}

VLA_ERROR VLAPI::read_db_and_declaration() {
  byte dbbbuffer[VLAPI_DBB_MEMSIZE];
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  err = dbbget(dbbbuffer,sizeof(dbbbuffer));
  if(err != VLA_ERR_NOERR)
    return err;

  DBB dbb1;
  memcpy(dbb1.block,dbbbuffer,sizeof(dbb1.block));
  memcpy(dbb1.fdf,dbbbuffer+DBB::FrmBeg,sizeof(dbb1.fdf));
  dbb1.open_dbb();

  // convert and write list of waypoints
  if (dbb1.header[0].dsfirst != 0xffff) {
    database.nwpts = 1 + (dbb1.header[0].dslast - dbb1.header[0].dsfirst)
      / dbb1.header[0].dslaenge;
    delete[] database.wpts;
    database.wpts = new WPT[database.nwpts];
    for (int i=0; i<database.nwpts; i++) {
      database.wpts[i].get(dbb1.block + dbb1.header[0].dsfirst +
                           i*dbb1.header[0].dslaenge);
    }
  }


  // convert and write list of routes
  if (dbb1.header[3].dsfirst != 0xffff) {
    database.nroutes = 1 + (dbb1.header[3].dslast - dbb1.header[3].dsfirst)
      / dbb1.header[3].dslaenge ;
    delete[] database.routes;
    database.routes = new ROUTE[database.nroutes];
    for (int i=0; i<database.nroutes; i++) {
      database.routes[i].get(dbb1.block + dbb1.header[3].dsfirst +
                             i*dbb1.header[3].dslaenge);
    }
  }

  // convert and write list of pilots
  if (dbb1.header[1].dsfirst != 0xffff) {
    database.npilots = 1 + (dbb1.header[1].dslast - dbb1.header[1].dsfirst)
      / dbb1.header[1].dslaenge;
    delete[] database.pilots;
    database.pilots = new PILOT[database.npilots];
    for (int i=0; i<database.npilots; i++) {
      database.pilots[i].get(dbb1.block + dbb1.header[1].dsfirst +
                             i*dbb1.header[1].dslaenge);
    }
  }

  declaration.get(&dbb1);

  return VLA_ERR_NOERR;
}


VLA_ERROR VLAPI::write_db_and_declaration() {

  DBB dbb1;

  dbb1.open_dbb();

  int i;
  for(i=0; i<database.nwpts; i++) {
    byte bwpt[13];
    database.wpts[i].put(bwpt);
    dbb1.add_ds(0,bwpt);
  }
  dbb1.close_db(0);

  for(i=0; i<database.npilots; i++) {
    byte bpilot[17];
    database.pilots[i].put(bpilot);
    dbb1.add_ds(1,bpilot);
  }
  dbb1.close_db(1);

  for(i=0; i<database.nroutes; i++) {
    byte broute[144];
    database.routes[i].put(broute);
    dbb1.add_ds(3,broute);
  }
  dbb1.close_db(3);

  declaration.put(&dbb1);

  // copy dbb1 blocks into buffer
  byte dbbbuffer[VLAPI_DBB_MEMSIZE];
  memcpy(dbbbuffer,dbb1.block,sizeof(dbb1.block));
  memcpy(dbbbuffer+DBB::FrmBeg,dbb1.fdf,sizeof(dbb1.fdf));
  // and write buffer back into VOLKSLOGGER
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;
  err = dbbput(dbbbuffer,sizeof(dbbbuffer));
  return err;
}

VLA_ERROR VLAPI::read_directory() {
  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR)
    return err;

  byte dirbuffer[VLAPI_LOG_MEMSIZE];
  err = readdir(dirbuffer, sizeof(dirbuffer));

  if(err == VLA_ERR_NOERR) {
    int fcount = conv_dir(0,dirbuffer,1);
    delete[] directory.flights;
    directory.flights = NULL;
    if(fcount>0) {
      directory.nflights = fcount;
      directory.flights = new DIRENTRY[fcount];
      conv_dir(directory.flights,dirbuffer,0);
      err = VLA_ERR_NOERR;
    }
    else {
      directory.nflights = 0;
      err = VLA_ERR_NOFLIGHTS;
    }
  }

  return err;

}

VLA_ERROR VLAPI::read_igcfile(char *filename, int index, int secmode) {
  FILE *outfile = fopen(filename,"wt");
  if(!outfile)
    return VLA_ERR_FILE;

  VLA_ERROR err = stillconnect();
  if(err != VLA_ERR_NOERR) {
    fclose(outfile);
    return err;
  }

  byte logbuffer[VLAPI_LOG_MEMSIZE];
  if (flightget(logbuffer, sizeof(logbuffer), index, secmode)>0)
    err = VLA_ERR_NOERR;

  word serno; long sp;
  long r;
  if(err == VLA_ERR_NOERR) {
    r = convert_gcs(0,outfile,logbuffer,1,&serno,&sp);
    if(r>0) {
      err = VLA_ERR_NOERR;
      print_g_record(
                     outfile,   // output to stdout
                     logbuffer, // binary file is in buffer
                     r          // length of binary file to include
                     );
    }
    else
      err = VLA_ERR_MISC;
  }
  fclose(outfile);
  return err;
}


// getting a waypoint object out of the database memory
//
void VLAPI_DATA::WPT::get(lpb p) {
  int32 ll;
  memcpy(name,p,6);
  name[6] = 0;
  strupr(name);
  typ = (WPTTYP) p[6] & 0x7f;
  ll =  (int32) (65536.0*(p[7]&0x7f) + 256.0 * p[8] + p[9]);
  lat = ll / 60000.0;
  if (p[7] & 0x80)
    lat = -lat;
  ll = (int32) (65536.0 * p[10] + 256 * p[11] + p[12]);
  lon = ll / 60000.0;
  if (p[6] & 0x80)
    lon = -lon;
}



// putting a waypoint object into the database memory
//
void VLAPI_DATA::WPT::put(lpb p) {
  int32 llat,llon;
  // String, evtl. mit Blanks aufgefüllt, zurückschreiben
  strupr(name);
  copy_padded(p, 6, name);
  // Koordinaten zurückschreiben
  llat = labs((long)(lat * 60000.0));
  llon = labs((long)(lon * 60000.0));
  p[6] = (typ&0x7f) | ((lon<0)?0x80:0);
  p[7] = (byte)((llat >> 16) | ((lat<0)?0x80:0));
  llat = llat & 0x0000ffff;
  p[8] = (byte) (llat >> 8);
  p[9] = (byte) (llat & 0x000000ff);
  p[10] = (byte) (llon >> 16);
  llon  = llon & 0x0000ffff;
  p[11] = (byte) (llon >> 8);
  p[12] = (byte) (llon & 0x000000ff);
}

// getting a declaration waypoint object out of the database
//
void VLAPI_DATA::DCLWPT::get(lpb p) {
  WPT::get(p);
  oztyp = (OZTYP)p[15];
  ws = p[13] * 2;
  if(oztyp == OZTYP_LINE) {
    lw = (p[14] & 0x0f) * ((p[14] & 0xf0) >> 4);
  }
  else {
    rz = (p[14] & 0x0f) * 100;
    rs = ((p[14] & 0xf0) >> 4) * 1000;
  }
}

// putting a declaration waypoint object into the database memory
//
void VLAPI_DATA::DCLWPT::put(lpb p) {
  WPT::put(p);
  p[15] = oztyp;
  p[13] = ws / 2;
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
    p[14] = (w1<<4) + w2;
  }
  else {
    p[14] = (rz / 100) + ( (rs/1000) << 4 );
  }
}

void VLAPI_DATA::ROUTE::get(lpb p) {
  memcpy(name,p,14);
  name[14] = 0;
  strupr(name);
  for(int i=0; i<10; i++)
    wpt[i].get(p + 14 + i*13);
}

void VLAPI_DATA::ROUTE::put(lpb p) {
	int i;
  strupr(name);
  copy_padded(p, 14, name);
  // In the following line, we insertes "int"
  // (Florian Ehinger)
  for(i=0; i<10; i++)
    wpt[i].put(p + 14 + i*13);
}

void VLAPI_DATA::PILOT::get(lpb p) {
  memcpy(name,p,16);
  name[16] = 0;
  strupr(name);
}

void VLAPI_DATA::PILOT::put(lpb p) {

  strupr(name);
  copy_padded(p, 15, name);
}


// read flight-declaration fields database into structure
//
void VLAPI_DATA::DECLARATION::get(DBB *dbb) {
  int16 i,p;
  char plt1[17];
  char plt2[17];
  char plt3[17];
  char plt4[17];
  plt1[0] = 0;
  plt2[0] = 0;
  plt3[0] = 0;
  plt4[0] = 0;
  if ((p = dbb->fdf_findfield(FLDPLT1))>=0)
    strncpy(plt1,(char*)(dbb->fdf+p+2),sizeof(plt1));
  if ((p = dbb->fdf_findfield(FLDPLT2))>=0)
    strncpy(plt2,(char*)(dbb->fdf+p+2),sizeof(plt2));
  if ((p = dbb->fdf_findfield(FLDPLT3))>=0)
    strncpy(plt3,(char*)(dbb->fdf+p+2),sizeof(plt3));
  if ((p = dbb->fdf_findfield(FLDPLT4))>=0)
    strncpy(plt4,(char*)(dbb->fdf+p+2),sizeof(plt4));
  flightinfo.pilot[0] = 0;
  strcat(flightinfo.pilot,plt1);
  strcat(flightinfo.pilot,plt2);
  strcat(flightinfo.pilot,plt3);
  strcat(flightinfo.pilot,plt4);
  if ((p = dbb->fdf_findfield(FLDGTY))>=0)
    strncpy(flightinfo.glidertype,(char*)(dbb->fdf+p+2),sizeof(flightinfo.glidertype));
  if ((p = dbb->fdf_findfield(FLDGID))>=0)
    strncpy(flightinfo.gliderid,(char*)(dbb->fdf+p+2),sizeof(flightinfo.gliderid));
  if ((p = dbb->fdf_findfield(FLDCCL))>=0)
    strncpy(flightinfo.competitionclass,(char*)(dbb->fdf+p+2),sizeof(flightinfo.competitionclass));
  if ((p = dbb->fdf_findfield(FLDCID))>=0)
    strncpy(flightinfo.competitionid,(char*)(dbb->fdf+p+2),sizeof(flightinfo.competitionid));
  if ((p = dbb->fdf_findfield(FLDTKF))>=0)
    flightinfo.homepoint.get((byte *)(dbb->fdf+p+2));

  if ((p = dbb->fdf_findfield(FLDSTA))>=0)
    task.startpoint.get((byte*)(dbb->fdf+p+2));
  if ((p = dbb->fdf_findfield(FLDFIN))>=0)
    task.finishpoint.get((byte*)(dbb->fdf+p+2));
  if ((p = dbb->fdf_findfield(FLDNTP))>=0)
    task.nturnpoints = (byte)dbb->fdf[p+2];
  for (i=0; i<task.nturnpoints; i++) {
    if ((p = dbb->fdf_findfield(FLDTP1+i))>=0)
      task.turnpoints[i].get((byte*)(dbb->fdf+p+2));
  }
}


void VLAPI_DATA::DECLARATION::put(DBB *dbb) {
  strupr(flightinfo.pilot);
  strupr(flightinfo.glidertype);
  strupr(flightinfo.gliderid);
  strupr(flightinfo.competitionclass);
  strupr(flightinfo.competitionid);

  char name[65];
  char name2[17];
  strncpy(name,flightinfo.pilot,sizeof(name));
  int i;
  for(i=0; i<4; i++) {
    strncpy(name2,name+16*i,16);
    name2[16] = 0;
    dbb->add_fdf(i+1,17,name2);
  }

  dbb->add_fdf(FLDGTY, strlen(flightinfo.glidertype)+1, flightinfo.glidertype);
  dbb->add_fdf(FLDGID, strlen(flightinfo.gliderid)+1, flightinfo.gliderid);
  dbb->add_fdf(FLDCCL, strlen(flightinfo.competitionclass)+1,
               flightinfo.competitionclass);
  dbb->add_fdf(FLDCID, strlen(flightinfo.competitionid)+1,
               flightinfo.competitionid);

  byte fdfwpt[16]; // temporary space for data conversions
  flightinfo.homepoint.put(fdfwpt);
  dbb->add_fdf(FLDTKF,sizeof(fdfwpt),&fdfwpt);

  byte ntp = task.nturnpoints;
  dbb->add_fdf(FLDNTP,1,&ntp);

  task.startpoint.put(fdfwpt);
  dbb->add_fdf(FLDSTA,sizeof(fdfwpt),&fdfwpt);

  task.finishpoint.put(fdfwpt);
  dbb->add_fdf(FLDFIN,sizeof(fdfwpt),&fdfwpt);

  for (i=0; i<task.nturnpoints; i++) {
    task.turnpoints[i].put(fdfwpt);
    dbb->add_fdf(FLDTP1+i,sizeof(fdfwpt),&fdfwpt);
  }
}
