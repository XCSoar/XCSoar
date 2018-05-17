/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef XCSOAR_IMI_TYPES_HPP
#define XCSOAR_IMI_TYPES_HPP

#include "Compiler.h"

#include <stdint.h>
#include <stddef.h>

namespace IMI
{
  typedef char IMICHAR;
  typedef uint8_t IMIBYTE;
  typedef uint16_t IMIWORD;
  typedef uint32_t IMIDWORD;
  typedef uint64_t IMIDDWORD;
  typedef int16_t IMISWORD;
  typedef int32_t IMISDWORD;
  typedef uint32_t IMIDATETIMESEC;

  enum TMsgType {
    MSG_ACK_SUCCESS      = 0x00,
    MSG_ACK_FAILURE      = 0x01,
    MSG_ACK_LOGGING      = 0x02,
    MSG_ACK_NOTCONFIG    = 0x03,
    MSG_ACK_INVSTATE     = 0x04,

    MSG_CFG_HELLO        = 0x10,
    MSG_CFG_BYE          = 0x11,
    MSG_CFG_FORCESTOP    = 0x12,
    MSG_CFG_STARTCONFIG  = 0x13,
    MSG_CFG_DEVICEINFO   = 0x14,
    MSG_CFG_KEEPCONFIG   = 0x15,
    MSG_CFG_CONFIG_ID    = 0x16,
    MSG_CFG_DEFAULTOZ    = 0x17,

    MSG_DECLARATION      = 0x20,

    MSG_FLASH = 0x30,
    MSG_FLASH_BULK = 0x31,

    MSG_FLIGHT_INFO      = 0x40,
    MSG_FLIGHT_DELETEALL = 0x42
  };

  // constants
  const IMIBYTE IMICOMM_SYNC_CHAR1 = 'E';
  const IMIBYTE IMICOMM_SYNC_CHAR2 = 'X';
  const unsigned IMICOMM_SYNC_LEN  = 2;
  const unsigned IMICOMM_CRC_LEN   = 2;
  const unsigned COMM_MAX_PAYLOAD_SIZE = 1024;
  const unsigned COMM_MAX_BULK_SIZE = 0xFFFF + 1;

  const unsigned IMIDECL_PLT_LENGTH = 30;
  const unsigned IMIDECL_CM2_LENGTH = 30;
  const unsigned IMIDECL_GTY_LENGTH = 20;
  const unsigned IMIDECL_GID_LENGTH = 12;
  const unsigned IMIDECL_CID_LENGTH = 4;
  const unsigned IMIDECL_CCL_LENGTH = 20;
  const unsigned IMIDECL_CLB_LENGTH = 20;
  const unsigned IMIDECL_SIT_LENGTH = 20;

  const unsigned IMIDECL_TASK_NAME_LENGTH = 30;

  const unsigned IMIDECL_WP_NAME_LENGTH   = 12;
  const unsigned IMIDECL_MAX_WAYPOINTS    = 15;

  const unsigned IMIRSA_MAX_BITS = 1024;

  const unsigned IMINO_ENL_MASK = 0x80;

  // messages
  struct TDeviceInfo {
    IMIBYTE device;
    IMIBYTE tampered;
    IMIBYTE hwVersion;
    IMIBYTE swVersion;
    IMIBYTE gps;
    IMIBYTE sensor;
    IMIBYTE flash;
    IMIBYTE eeprom;
    IMIDWORD flashSize;
    IMIDWORD eepromSize;
    IMISWORD sensor0Offset;
    IMISWORD sensor8kOffset;
    IMIWORD buildNumber;
    IMIBYTE reserved[64 - 22];
  } gcc_packed;

  struct TDeclarationHeader {
    /** Device type index */
    IMIBYTE id, device;
    /** Serial number */
    IMIWORD sn;
    IMIDWORD flightNumber;
    /** Hardware version */
    IMIBYTE hwVersion;
    /** Firmware version */
    IMIBYTE swVersion;
    /** GPS module index */
    IMIBYTE gps;
    /** Baro sensor index with ENL bit */
    IMIBYTE sensor;
    /** Flight date */
    IMIDATETIMESEC date;
    /** Pilot name */
    IMICHAR plt[IMIDECL_PLT_LENGTH];
    /** Pilot birthday */
    IMIBYTE db1Day, db1Month;
    IMIWORD db1Year;
    /** Second crew member name */
    IMICHAR cm2[IMIDECL_CM2_LENGTH];
    /** Second crew member birthday */
    IMIBYTE db2Day, db2Month;
    IMIWORD db2Year;
    /** Glider type */
    IMICHAR gty[IMIDECL_GTY_LENGTH];
    /** Glider ID */
    IMICHAR gid[IMIDECL_GID_LENGTH];
    /** Competition ID */
    IMICHAR cid[IMIDECL_CID_LENGTH];
    /** Competition class */
    IMICHAR ccl[IMIDECL_CCL_LENGTH];
    /** Club name */
    IMICHAR clb[IMIDECL_CLB_LENGTH];
    /** Club site */
    IMICHAR sit[IMIDECL_SIT_LENGTH];
    /** Task name */
    IMICHAR tskName[IMIDECL_TASK_NAME_LENGTH];
    /** Task number */
    IMIWORD tskNumber;
    /** Task date */
    IMIBYTE tskDay, tskMonth;
    IMIWORD tskYear;
    /** Start time of the recording */
    IMIDATETIMESEC recStartDateTime;
    /** n-th flight of the day */
    IMIWORD flightOfDay;
    IMIWORD reserved1;
    IMIDATETIMESEC flightStartDateTime;
    IMIBYTE reserved2[28];
  } gcc_packed;


  struct TObservationZone {
    /**
     * 0 -> default, 1-5 -> direction of course, the same value as in SeeYou
     *
     * 0 - default = ignore observation zone setting and use
     *               default OZ stored in Erixx
     * 1 - fixed angle
     * 2 - symmetrical (invalid for start and finish WP)
     * 3 - to next point (invalid for finish WP)
     * 4 - to prev point (invalid of start WP)
     * 5 - to start point (invalid for start WP)
     */
    IMIDWORD style:3;

    /** angle * 10, 0-180 degrees, values 0-1800 (= angle modulo 180 * 10) */
    IMIDWORD A1:11;
    /** radius in meters (max. radius 250km) */
    IMIDWORD R1:18;

    /** reduce leg distance (for cylinder for example) */
    IMIDWORD reduce:1;
    /** currently not used in Erixx */
    IMIDWORD move:1;
    /** Line only (not cylinder nor sector, angle is ignored) */
    IMIDWORD line_only:1;

    /** angle * 10, 0-180 degrees, values 0-1800 (= angle modulo 180 * 10) */
    IMIDWORD A2:11;
    /** radius in meters (max. radius 250km) */
    IMIDWORD R2:18;

    /**
     * angle * 10, 0,0-360,0 (modulo 360 * 10),
     * used when style = 1 = fixed value
     */
    IMIDWORD A12:12;

    /** maximum altitude of OZ in meters (0-16km). 0 =ignore maximum altitude */
    IMIDWORD maxAlt: 14;

    IMIDWORD reserved: 6;
  } gcc_packed;


  struct TWaypoint {
    IMIDWORD lon:25;
    IMIDWORD reserved1:7;

    IMIDWORD lat:25;
    IMIDWORD reserved2:7;

    IMICHAR name[IMIDECL_WP_NAME_LENGTH];

    TObservationZone oz;
  } gcc_packed;


  struct TDeclaration {
    TDeclarationHeader header;
    TWaypoint wp[IMIDECL_MAX_WAYPOINTS];
    IMIBYTE reserved[sizeof(TWaypoint) - sizeof(IMIWORD)];
    IMIWORD crc16;
  } gcc_packed;


  struct TMsg {
    IMIBYTE syncChar1, syncChar2;
    IMIWORD sn;
    IMIBYTE msgID, parameter1;
    IMIWORD parameter2;
    IMIWORD parameter3;
    IMIWORD payloadSize;
    IMIBYTE payload[COMM_MAX_PAYLOAD_SIZE];
    IMIWORD crc16;
  } gcc_packed;

  struct FlightInfo
  {
    IMIDWORD address;
    /** Serial number */
    IMIWORD sn;
    /** n-th flight of the day */
    IMIWORD flightOfDay;
    /** Start date and time */
    IMIDATETIMESEC start;
    /** Landing date and time */
    IMIDATETIMESEC finish;
    /** Pilot name */
    IMICHAR plt[IMIDECL_PLT_LENGTH];
    /** Glider ID */
    IMICHAR gid[IMIDECL_GID_LENGTH];
    /** Glider type */
    IMICHAR gty[IMIDECL_GTY_LENGTH];
    /** Competition ID */
    IMICHAR cid[IMIDECL_CID_LENGTH];
    IMIBYTE reserved[96 - 82];
  } gcc_packed;

  struct FlightFinish
  {
    IMIWORD stop;
    IMIWORD reserved;
    IMIDATETIMESEC recStopDateTime;
    IMIDWORD fixes;
    IMIDWORD fixes2;
    IMIDWORD lastLat;
    IMIDWORD lastLon;
    IMIDATETIMESEC flightStopDateTime;
    IMIBYTE reserved2[66];
    IMIWORD crc16;
  } gcc_packed;

  struct Signature
  {
    IMIWORD hashBits;
    IMIWORD rsaBits;
    IMIBYTE signature[IMIRSA_MAX_BITS / 8];
    IMIBYTE reserved[160 - (IMIRSA_MAX_BITS / 8) - 7];
    IMIBYTE tampered;
    IMIWORD crc16;
  } gcc_packed;

  struct Flight
  {
    TDeclaration decl;
    FlightFinish finish;
    Signature signature;
  } gcc_packed;

  #define IMIFIX_ID_DELETED     0   //000
  #define IMIFIX_ID_IMIDECLARATION 1   //001
  #define IMIFIX_ID_B_RECORD    2   //010
  #define IMIFIX_ID_B2_RECORD   3   //011
  #define IMIFIX_ID_E_RECORD    4   //100
  #define IMIFIX_ID_K_RECORD    5   //101
  #define IMIFIX_ID_X_RECORD    6   //110 // reserved
  #define IMIFIX_ID_FREE        7   //111

  #define IMIIS_FIX(id) (id >= IMIFIX_ID_B_RECORD && id <= IMIFIX_ID_X_RECORD)

  struct Fix
  {
    IMIDWORD id:3;
    IMIDWORD time:17;
    IMIDWORD padding:12;
    IMIBYTE body[11];
    IMIBYTE checksum;
  } gcc_packed;

  struct FixB
  {
    IMIDWORD id:3;
    IMIDWORD time:17;
    IMIDWORD fv:2;
    IMIDWORD enl:10;

    IMIDDWORD lat:25;
    IMIDDWORD lon:25;
    IMIDDWORD gpsalt:14;

    IMIDWORD alt:14;
    IMIDWORD fxa:10;
    IMIDWORD checksum:8;
  } gcc_packed; // B record (id = 2)

  struct FixB2
  {
    IMIDWORD id:3;
    IMIDWORD time1:5;
    IMIDWORD time2:4;
    IMIDWORD enl1:10;
    IMIDWORD enl2:10;

    IMISDWORD lat1:11;
    IMISDWORD lon1:11;
    IMISDWORD lat2:10;

    IMISDWORD lon2:10;
    IMISDWORD alt1:8;
    IMISDWORD alt2:7;
    IMISDWORD gpsalt2:7;

    IMISDWORD gpsalt1:8;
    IMISDWORD fxa1:8;
    IMISDWORD fxa2:8;
    IMIDWORD checksum:8;
  } gcc_packed; // B2 record (id = 3)

  #define IMIFIX_E_TYPE_SATELLITES 0
  #define IMIFIX_E_TYPE_COMMENT    1
  #define IMIFIX_E_TYPE_PEV        2
  #define IMIFIX_E_TYPE_TASK       3

  struct FixE
  {
    IMIBYTE id:3;
    IMIBYTE time1:5;
    IMIBYTE time2:8;
    IMIBYTE time3:4;
    IMIBYTE type:4;

    IMIBYTE text[12];

    IMIBYTE checksum;
  } gcc_packed; // Event, Satellites, Comment (id = 5)

  struct FixK
  {
    IMIDWORD id:3;
    IMIDWORD time:17;
    IMIDWORD temp:12;

    IMIDWORD vext:8;
    IMIDWORD gsp:10;
    IMIDWORD pressure:14;

    IMIDWORD hdt:9;
    IMIDWORD reserved1:23;

    IMIBYTE reserved2[3];
    IMIBYTE checksum;
  } gcc_packed;
}

#define IMICOMM_MSG_HEADER_SIZE ((size_t)(&(((TMsg *)nullptr)->payload)))
#define IMICOMM_MAKEBIGPARAM(param1, param2) ((((unsigned)(param1 & 0xFF)) << 16) | (unsigned)(param2 & 0xFFFF))
#define IMICOMM_BIGPARAM1(param) ((IMIBYTE)((param) >> 16))
#define IMICOMM_BIGPARAM2(param) ((IMIWORD)(param))

#endif
