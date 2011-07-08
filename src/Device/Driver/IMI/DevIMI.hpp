/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#ifndef XCSOAR_IMI_DEVIMI_HPP
#define XCSOAR_IMI_DEVIMI_HPP

#include "Compiler.h"
#include "Types.hpp"

#include <stdint.h>
#include <stddef.h>

class Port;
struct Declaration;

/**
 * @brief IMI-Gliding ERIXX device class
 *
 * Class provides support for IMI-Gliding ERIXX IGC certifed logger.
 *
 * @note IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */
namespace IMI
{
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

    MSG_FLIGHT_INFO      = 0x40,
    MSG_FLIGHT_DELETEALL = 0x42
  };

  // messages
  struct TDeviceInfo;
  struct TDeclarationHeader;
  struct TObservationZone;
  struct TWaypoint;
  struct TDeclaration;
  struct TMsg;

  // helpers
  struct TAngle;

  // message parser
  class CMsgParser;

  // constants
  const IMIBYTE IMICOMM_SYNC_CHAR1 = 'E';
  const IMIBYTE IMICOMM_SYNC_CHAR2 = 'X';
  const unsigned IMICOMM_SYNC_LEN  = 2;
  const unsigned IMICOMM_CRC_LEN   = 2;
  const unsigned COMM_MAX_PAYLOAD_SIZE = 1024;

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

  bool DeclareTask(Port &port, const Declaration &declaration);
  void Register();
};


/* *********************** M E S S A G E S ************************** */

struct IMI::TDeviceInfo {
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


struct IMI::TDeclarationHeader {
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


struct IMI::TObservationZone {
  IMIDWORD style:3;        // 0 -> default, 1-5 -> direction of course, the same value as in SeeYou
                           //0 - default = ignore observation zone setting and use default OZ stored in Erixx
                           //1 - fixed angle
                           //2 - symmetrical (invalid for start and finish WP)
                           //3 - to next point (invalid for finish WP)
                           //4 - to prev point (invalid of start WP)
                           //5 - to start point (invalid for start WP)

  IMIDWORD A1:11;          // angle * 10, 0-180 degrees, values 0-1800 (= angle modulo 180 * 10)
  IMIDWORD R1:18;          // radius in meters (max. radius 250km)

  IMIDWORD reduce:1;       // = reduce leg distance (for cylinder for example)
  IMIDWORD move:1;         // = currently not used in Erixx
  IMIDWORD line_only:1;    // = Line only (not cylinder nor sector, angle is ignored)

  IMIDWORD A2:11;          // angle * 10, 0-180 degrees, values 0-1800 (= angle modulo 180 * 10)
  IMIDWORD R2:18;          // radius in meters (max. radius 250km)

  IMIDWORD A12:12;         // angle * 10, 0,0-360,0 (modulo 360 * 10), used when style = 1 = fixed value
  IMIDWORD maxAlt: 14;     // maximum altitude of OZ in meters (0-16km). 0 =ignore maximum altitude

  IMIDWORD reserved: 6;
} gcc_packed;


struct IMI::TWaypoint {
  IMIDWORD lon:25;
  IMIDWORD reserved1:7;

  IMIDWORD lat:25;
  IMIDWORD reserved2:7;

  IMICHAR name[IMIDECL_WP_NAME_LENGTH];

  TObservationZone oz;
} gcc_packed;


struct IMI::TDeclaration {
  TDeclarationHeader header;
  TWaypoint wp[IMIDECL_MAX_WAYPOINTS];
  IMIBYTE reserved[sizeof(TWaypoint) - sizeof(IMIWORD)];
  IMIWORD crc16;
} gcc_packed;


struct IMI::TMsg {
  IMIBYTE syncChar1, syncChar2;
  IMIWORD sn;
  IMIBYTE msgID, parameter1;
  IMIWORD parameter2;
  IMIWORD parameter3;
  IMIWORD payloadSize;
  IMIBYTE payload[COMM_MAX_PAYLOAD_SIZE];
  IMIWORD crc16;
} gcc_packed;

#define IMICOMM_MSG_HEADER_SIZE ((size_t)(&(((TMsg *)NULL)->payload)))
#define IMICOMM_BIGPARAM1(param) ((IMIBYTE)((param) >> 16))
#define IMICOMM_BIGPARAM2(param) ((IMIWORD)(param))

#endif
