/***********************************************************************
**
**   vla_support.cpp
**
**   This file is part of libkfrgcs.
**
************************************************************************
**
**   Copyright (c):  2002 by Heiner Lamprecht,
**                           Garrecht Ingenieurgesellschaft
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef VLA_SUPPORT
#define VLA_SUPPORT

#include "vlapityp.h"
#include "vlapierr.h"

#include <stddef.h>

class Port;
class OperationEnvironment;

/*
	VLA_SYS contains target system dependent primitives upon which
	the subclasses rely.
	Implement this functions according to	your target system.
	A sample implementation for Win32(R) can be found in file
		"VLAPI2SYS_WIN32.CPP"
*/
class VLA_SYS {
protected:
	// Text-IDs for state and error output messages
	enum VLS_TXT_ID {
		VLS_TXT_SENDCMD = 0,
		VLS_TXT_WTCMD,
		VLS_TXT_XFERRING,
		VLS_TXT_WDB,

		VLS_TXT_UIRQ,
		VLS_TXT_CRC,
		VLS_TXT_EMPTY,
		VLS_TXT_UIRQ2,
		VLS_TXT_CONNECT,
		VLS_TXT_CONN_OK,
		VLS_TXT_CONN_FL,
		VLS_TXT_NIL,
		VLS_TXT_BADCMD,
		VLS_TXT_WRONGFR,
		VLS_TXT_NOFR
	};

	// Timer functions
  void wait_ms(const int32 t);

	// serial port functions
  VLA_ERROR serial_set_baudrate(const int32 baudrate);
  VLA_ERROR serial_out(const byte outbyte);
	VLA_ERROR serial_in(byte *inbyte);
	VLA_ERROR serial_empty_io_buffers();

  // user interaction functions
	boolean test_user_break();
  int16 clear_user_break();
  void progress_reset();
  void progress_set(VLS_TXT_ID);
  void show(VLS_TXT_ID);
  void showwait(VLS_TXT_ID);

  //
  Port *port;

 VLA_SYS(Port &_port):port(&_port) {}
};


/*
	This class contains target system independent base functions
	for communication with the VOLKSLOGGER.
	These functions are critical and normally don't need to be changed
	by the user. If you think that something has to be changed here,
	contact GARRECHT for comments on your intended changes.
*/
class VLA_XFR : protected VLA_SYS {
protected:
	// flow- and format- control
	enum {
		STX = 0x02,
		ETX = 0x03,
		ENQ = 0x05,
		ACK = 0x06,
		DLE = 0x10,
		NAK = 0x15,
		CAN = 0x18
	};
	// Kommandos PC -> Logger
	enum {
		cmd_INF = 0x00,// Information
		cmd_DIR = 0x01,// Verzeichnis lesen
		cmd_GFL = 0x02,// Flug lesen mit MD4
		cmd_GFS = 0x03,// Flug lesen mit Signatur
		cmd_RDB = 0x04,// Datenbank lesen
		cmd_WPR = 0x05,// Parameter schreiben
		cmd_CFL = 0x06,// Flugspeicher löschen
		cmd_PDB = 0x07,// Datenbank schreiben
		cmd_SIG = 0x08,// Signatur berechnen und ausgeben
		cmd_ERO = 0x09,// Emergency readout (Memorydump lesen)
		cmd_RST = 0x0c // Restart Logger
	};

	static int32 commandbaud;    // baudrate for commands

  OperationEnvironment &env;

  int32 databaud; // Baudrate as integer (e.g. 115200)
  int databaudidx; // Baudrate as index for VOLKSLOGGER

  bool SendWithCRC(const void *data, size_t length);

  // send command to VL
  int16 sendcommand(byte cmd, byte param1, byte param2);
  // wait for acknowledgement of command (if no data is expected)
  int16 wait4ack();
  // read block of data from VL, with flow-control
  int32 readlog(lpb puffer, int32);

  VLA_XFR(Port &port, OperationEnvironment &env);
  void set_databaud(int32 db);

	// establish connection with VL within specified time
  VLA_ERROR connect(int32, int quietmode = 0);

	VLA_ERROR readinfo(lpb buffer, int32 buffersize);

  // write binary DATABASE-block to VL
  VLA_ERROR dbbput(lpb dbbbuffer, int32 dbbsize);
  // read binary DATABASE from VL
  VLA_ERROR dbbget(lpb dbbbuffer, int32 dbbsize);
  // read all binary flight logs from VL
  VLA_ERROR all_logsget(lpb dbbbuffer, int32 dbbsize);
  // read one binary flight log from VL
  int32 flightget(lpb buffer, int32 buffersize, int16 flightnr, int16 secmode);
  VLA_ERROR readdir(lpb buffer, int32 buffersize);
};

#endif
