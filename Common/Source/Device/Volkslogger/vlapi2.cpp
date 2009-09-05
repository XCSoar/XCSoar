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

#include <memory.h>
#include <string.h>
#include <stdlib.h>


#include "Device/Volkslogger/vlapi2.h"
#include "Device/Volkslogger/vlapierr.h"
#include "Device/Volkslogger/dbbconv.h"
#include "Device/Volkslogger/vlconv.h"
#include "Device/Volkslogger/grecord.h"

#include "Device/Volkslogger/utils.h"

extern int noninteractive;

// sizes of VL memory regions
const int VLAPI_DBB_MEMSIZE = 16384;
const int32 VLAPI_LOG_MEMSIZE = 81920L;


void VLAPI::set_device(PDeviceDescriptor_t	d) {
  device = d;
}


// ------------------------------------------------------------
//                        VLA_XFR
// ------------------------------------------------------------

int32 VLA_XFR::commandbaud = 9600L;

word VLA_XFR::Crc16Table[256] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
  0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
  0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
  0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
  0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
  0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
  0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
  0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
  0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
  0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
  0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
  0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
  0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
  0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
  0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
  0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
  0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
  0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
  0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
  0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
  0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
  0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
  0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
  0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
  0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
  0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
  0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
  0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
  0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
  0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
  0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};


word VLA_XFR::UpdateCRC(byte Octet,word CRC) {
  return  0xffff & ( (CRC << 8) ^ Crc16Table[ (CRC >> 8) ^ Octet ] );
}

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


// send command to VOLKSLOGGER
//
int16 VLA_XFR::sendcommand(byte cmd, byte param1, byte param2) {
  byte	        c=255;
  const int16   d = 2;  //Verzögerungszeit 2ms
  byte 		cmdarray[8];
  word          crc16 = 0;
  int32	 	t1;
  int16         timeout = 4;
  // alte Zeichen verwerfen
  wait_ms(100);
  serial_empty_io_buffers();

  // Kommandointerpreter im VL zurücksetzen
  for (unsigned i =0 ; i < 6; i++) {
    serial_out(CAN);
    wait_ms(d);
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
  wait_ms(d);
  for (unsigned i = 0; i < sizeof(cmdarray); i++) {
    crc16 = UpdateCRC(cmdarray[i],crc16);
    serial_out(cmdarray[i]);
    wait_ms(d);
  }
  serial_out(crc16/256);
  wait_ms(d);
  serial_out(crc16%256);
  wait_ms(d);
  // Kommandobestätigung abwarten, aber höchstens timeout Sekunden
  t1 = get_timer_s()+timeout;
  while( serial_in(&c) && (get_timer_s()<t1) )
    progress_set(VLS_TXT_SENDCMD);
  // Timeoutbehandlung
  if (get_timer_s() >= t1)
    c = 255;
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
  case 255:
    showwait(VLS_TXT_NOFR);
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
  int32 t1;
  int16 timeout = 180;
  // Anfangszeit merken
  t1 = get_timer_s();
  // Auf Beendigungscode vom Logger warten
  while ( !test_user_break() && serial_in(&c) && get_timer_s()<t1+timeout )
    progress_set(VLS_TXT_WTCMD);
  if (test_user_break()) {
    if (clear_user_break() == 1)
      return 255;
  }
  else if (get_timer_s() >= t1+timeout)
    return 255;
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
  wait_ms(300);
  while(!ende) {
    // Zeichen anfordern und darauf warten
    serial_out(ACK);

    pp++;
    serial_in(&c);
    if ((gcs_counter>=maxlen) && (crc16==0)) {
      ende = 1;
    }

    // dabei ist Benutzerabbruch jederzeit möglich
    if (test_user_break()) {
      if (clear_user_break() == 1) {
        ende = -1;
        wait_ms(10);
        serial_out(CAN);
        serial_out(CAN);
        serial_out(CAN);
      }
    }
    // oder aber das empfangene Zeichen wird ausgewertet
    else {
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
    }


    if (gcs_counter == maxlen) {
      //    ende = 1; // JMW cheat quick exit... TODO fix me
    }

  }
  wait_ms(100);

  if (ende == -1) {
    show(VLS_TXT_UIRQ);
    gcs_counter = 0;
  }
  else if (crc16) {
    show(VLS_TXT_CRC);
    gcs_counter = 0;
  }
  else if (gcs_counter > 2) {              //CRC am Ende abschneiden
    gcs_counter -= 2;
    p--;
    p--;
    if (gcs_counter < maxlen)
      p[0] = 0xff;
    if (gcs_counter+1 < maxlen)
      p[1] = 0xff;
    p++;
    p++;
  }
  else {
    show(VLS_TXT_EMPTY);
    gcs_counter = 0;
  }

  if ((ende == -1) || crc16)
    return -1; //Fehlermeldung
  else
    return gcs_counter;
}


// Blockweises Schreiben in den Logger (z.B. Datenbank)
VLA_ERROR VLA_XFR::dbbput(lpb dbbbuffer, int32 dbbsize) {
  word crc16;
  byte c;
  int32 i;
  int32 td = 1;
  int32 step;

  // Schreibkommando geben
  serial_empty_io_buffers();
  sendcommand(cmd_PDB,0,0); // muß noch mit Timeout versehen werden
  // auf Löschende warten
  while ( serial_in(&c) && !test_user_break() );
  // Fehlerbehandlung
  if (test_user_break())
    if (clear_user_break() == 1) {
      showwait(VLS_TXT_UIRQ2);
      return VLA_ERR_USERCANCELED;
    }
  if (c != ACK)
    return VLA_ERR_MISC;
  // Schreiben der Datenbank
  wait_ms(100);
  crc16 = 0;
  step = dbbsize / 400;

  for(i=0; i<dbbsize; i++) {
    c = dbbbuffer[i];
    crc16 = UpdateCRC(c,crc16);
    serial_out(c);
    if((i%step)==0)
      progress_set(VLS_TXT_WDB);
  }

  serial_out(crc16/256);
  wait_ms(td);
  serial_out(crc16%256);
  wait_ms(td);
  // auf Bestätigung warten
  while ( serial_in(&c) && !test_user_break());
  // Fehlerbehandlung
  if (test_user_break()) {
    if (clear_user_break() == 1) {
      showwait(VLS_TXT_UIRQ2);
      return VLA_ERR_USERCANCELED;
    }
  }
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
  wait_ms(300);
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
  wait_ms(300);
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
  wait_ms(300);
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
  int16 timeout = 0;
  int32 stoptime;
  int16 i;
  VLA_ERROR rc = VLA_ERR_NOERR;
  byte c;

  if(!quietmode)
    show(VLS_TXT_CONNECT);

  serial_empty_io_buffers();
  // eventuell noch laufende Aktion im Logger abbrechen
  for (i=0; i<10; i++) {
    serial_out(CAN);
    wait_ms(1);
  }
  c = 0;
  timeout = 0;

  stoptime = get_timer_s() + waittime;

  do { // Solange R's aussenden, bis ein L zurückkommt
    serial_out('R');
    wait_ms(30);
    if (get_timer_s() >= stoptime)
      timeout = 1;
  } while ( (!timeout) && (serial_in(&c) || c != 'L' ));

  if (timeout)
    rc = VLA_ERR_NOANSWER;
  else { // Ab dann:
    l_count = 1;
    do { // Auf 4 hintereinanderfolgende L's warten
      if (!serial_in(&c)) {
        if (c == 'L') {
          l_count++;
          if (l_count >= 4)
            break;
        }
        else {
          rc = VLA_ERR_NOANSWER;
          break;
        }
      }
      if (get_timer_s() >= stoptime)
        timeout = 1;
    } while ( !timeout && !serial_in(&c) );
    if (timeout)
      rc = VLA_ERR_TIMEOUT;
  }

  if(!quietmode) {
    if (rc == VLA_ERR_NOERR)
      show(VLS_TXT_CONN_OK);
    else
      show(VLS_TXT_CONN_FL);
  }
  wait_ms(300);
  serial_empty_io_buffers();
  return rc;
}

VLA_XFR::VLA_XFR() {
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
VLAPI::VLAPI() {


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
  // aquire port from OS
  if ((err = serial_open_port()) == VLA_ERR_NOERR) {
    // setup port
    if ((err = serial_set_baudrate(commandbaud)) == VLA_ERR_NOERR) {
      set_databaud(sbaudrate);
      // connect
      if(connectit) {
        if((err = connect(timeout,quiet)) == VLA_ERR_NOERR) {
          vlpresent = 1;
          // nach erfolgreichem Connect
          // noninteractive wieder auf "Still" setzen
          //noninteractive = 1;
        }
      }
    }
  }

  if(err == VLA_ERR_COMM)
    serial_close_port();

  return err;
}



// close connection to VL
void VLAPI::close(boolean reset) {
  if(vlpresent) {
    if(reset) {
      sendcommand(cmd_RST,0,0);
    }
    serial_close_port();
    vlpresent = 0;
  }
}


VLA_ERROR VLAPI::read_info() {
  byte buffer[8];
  VLA_ERROR err;

  if(((err = readinfo(buffer,sizeof(buffer))) == VLA_ERR_NOERR)) {
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
    if(database.wpts != 0) {
      delete[] database.wpts;
      database.wpts = 0;
    }
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
    if(database.routes != 0) {
      delete[] database.routes;
      database.routes = 0;
    }
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
    if(database.pilots != 0) {
      delete[] database.pilots;
      database.pilots = 0;
    }
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
    if(directory.flights != 0) {
      delete[] directory.flights;
      directory.flights = 0;
    }
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
  if(err != VLA_ERR_NOERR)
    return err;

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
  int16 i,l;
  // String, evtl. mit Blanks aufgefüllt, zurückschreiben
  strupr(name);
  memcpy(p,name,6);
  l = strlen((char *)p);
  for(i=l; i<6; i++)
    p[i] = ' ';
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
  memcpy(p,name,14);
  for(i=strlen((char *)p); i<14; i++)
    p[i] = ' ';
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
  memcpy(p,name,16);
  for(int i=strlen((char *)p); i<16; i++)
    p[i] = ' ';

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
