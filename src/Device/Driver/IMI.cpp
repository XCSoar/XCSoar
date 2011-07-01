/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights
*/

/**
 * IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */

#include "Device/Driver/IMI.hpp"
#include "Device/Driver.hpp"
#include "Device/Port.hpp"
#include "OS/Clock.hpp"
#include "Compiler.h"

#include <stdint.h>

#ifdef _UNICODE
#include <windows.h>
#endif

static void
unicode2usascii(const TCHAR* unicode, char* ascii, int outSize)
{
#ifdef _UNICODE
  WideCharToMultiByte(CP_ACP, 0, unicode, -1, ascii, outSize, "?", NULL);
#else
  strncpy(ascii, unicode, outSize - 1);
  ascii[outSize - 1] = 0;
#endif
}

/**
 * @brief IMI-Gliding ERIXX device class
 *
 * Class provides support for IMI-Gliding ERIXX IGC certifed logger.
 *
 * @note IMI driver methods are based on the source code provided by Juraj Rojko from IMI-Gliding.
 */
class CDevIMI
{
  /** 8bit text character */
  typedef char IMICHAR;
  typedef uint8_t IMIBYTE;
  typedef uint16_t IMIWORD;
  typedef uint32_t IMIDWORD;
  typedef int16_t IMISWORD;
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
  static const IMIBYTE IMICOMM_SYNC_CHAR1 = 'E';
  static const IMIBYTE IMICOMM_SYNC_CHAR2 = 'X';
  static const unsigned IMICOMM_SYNC_LEN  = 2;
  static const unsigned IMICOMM_CRC_LEN   = 2;
  static const unsigned COMM_MAX_PAYLOAD_SIZE = 1024;

  // variables
  static bool _connected;
  static CMsgParser _parser;
  static TDeviceInfo _info;
  static IMIWORD _serialNumber;

  // IMI tools
  static IMIWORD CRC16Checksum(const void *message, unsigned bytes);
  static void IMIWaypoint(const Declaration &decl, unsigned imiIdx, TWaypoint &imiWp);
  static bool Send(Port &d, const TMsg &msg);
  static bool Send(Port &d,
                   IMIBYTE msgID, const void *payload = 0, IMIWORD payloadSize = 0,
                   IMIBYTE parameter1 = 0, IMIWORD parameter2 = 0, IMIWORD parameter3 = 0);
  static const TMsg *Receive(Port &d,
                             unsigned extraTimeout, unsigned expectedPayloadSize);
  static const TMsg *SendRet(Port &d,
                             IMIBYTE msgID, const void *payload, IMIWORD payloadSize,
                             IMIBYTE reMsgID, IMIWORD retPayloadSize,
                             IMIBYTE parameter1 = 0, IMIWORD parameter2 = 0, IMIWORD parameter3 = 0,
                             unsigned extraTimeout = 300, int retry = 4);

  // IMI interface
  static bool Connect(Port &d);
  static bool DeclarationWrite(Port &d, const Declaration &decl);
  static bool Disconnect(Port &d);

public:
  static bool DeclareTask(Port &d, const Declaration &declaration);
  static void Register();
};

/* *********************** C O N S T A N T S ************************** */

static const unsigned IMIDECL_PLT_LENGTH = 30;
static const unsigned IMIDECL_CM2_LENGTH = 30;
static const unsigned IMIDECL_GTY_LENGTH = 20;
static const unsigned IMIDECL_GID_LENGTH = 12;
static const unsigned IMIDECL_CID_LENGTH = 4;
static const unsigned IMIDECL_CCL_LENGTH = 20;
static const unsigned IMIDECL_CLB_LENGTH = 20;
static const unsigned IMIDECL_SIT_LENGTH = 20;

static const unsigned IMIDECL_TASK_NAME_LENGTH = 30;

static const unsigned IMIDECL_WP_NAME_LENGTH   = 12;
static const unsigned IMIDECL_MAX_WAYPOINTS    = 15;



/* *********************** M E S S A G E S ************************** */

struct CDevIMI::TDeviceInfo {
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


struct CDevIMI::TDeclarationHeader {
  IMIBYTE id, device;
  IMIWORD sn;
  IMIDWORD flightNumber;
  IMIBYTE hwVersion;
  IMIBYTE swVersion;
  IMIBYTE gps;
  IMIBYTE sensor;
  IMIDATETIMESEC date;
  IMICHAR plt[IMIDECL_PLT_LENGTH];
  IMIBYTE db1Day, db1Month;
  IMIWORD db1Year;
  IMICHAR cm2[IMIDECL_CM2_LENGTH];
  IMIBYTE db2Day, db2Month;
  IMIWORD db2Year;
  IMICHAR gty[IMIDECL_GTY_LENGTH];
  IMICHAR gid[IMIDECL_GID_LENGTH];
  IMICHAR cid[IMIDECL_CID_LENGTH];
  IMICHAR ccl[IMIDECL_CCL_LENGTH];
  IMICHAR clb[IMIDECL_CLB_LENGTH];
  IMICHAR sit[IMIDECL_SIT_LENGTH];
  IMICHAR tskName[IMIDECL_TASK_NAME_LENGTH];
  IMIWORD tskNumber;
  IMIBYTE tskDay, tskMonth;
  IMIWORD tskYear;
  IMIDATETIMESEC recStartDateTime;
  IMIWORD flightOfDay;
  IMIWORD reserved1;
  IMIDATETIMESEC flightStartDateTime;
  IMIBYTE reserved2[28];
} gcc_packed;


struct CDevIMI::TObservationZone {
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


struct CDevIMI::TWaypoint {
  IMIDWORD lon:25;
  IMIDWORD reserved1:7;

  IMIDWORD lat:25;
  IMIDWORD reserved2:7;

  IMICHAR name[IMIDECL_WP_NAME_LENGTH];

  TObservationZone oz;
} gcc_packed;


struct CDevIMI::TDeclaration {
  TDeclarationHeader header;
  TWaypoint wp[IMIDECL_MAX_WAYPOINTS];
  IMIBYTE reserved[sizeof(TWaypoint) - sizeof(IMIWORD)];
  IMIWORD crc16;
} gcc_packed;


struct CDevIMI::TMsg {
  IMIBYTE syncChar1, syncChar2;
  IMIWORD sn;
  IMIBYTE msgID, parameter1;
  IMIWORD parameter2;
  IMIWORD parameter3;
  IMIWORD payloadSize;
  IMIBYTE payload[COMM_MAX_PAYLOAD_SIZE];
  IMIWORD crc16;
} gcc_packed;

#define IMICOMM_MAX_MSG_SIZE (sizeof(TMsg))
#define IMICOMM_MSG_HEADER_SIZE ((size_t)(&(((TMsg *)NULL)->payload)))
#define IMICOMM_BIGPARAM1(param) ((IMIBYTE)((param) >> 16))
#define IMICOMM_BIGPARAM2(param) ((IMIWORD)(param))




/* *********************** M S G   P A R S E R ************************** */

/**
 * @brief Message parser class
 */
class CDevIMI::CMsgParser {
  /**
   * @brief Parser state
   */
  enum TState {
    STATE_NOT_SYNC,                               /**< @brief Synchronization bits not found */
    STATE_COMM_MSG                                /**< @brief Parsing message body */
  };

  TState _state;                                  /**< @brief Parser state */
  IMIBYTE _msgBuffer[IMICOMM_MAX_MSG_SIZE];       /**< @brief Parsed message buffer */
  unsigned _msgBufferPos;                         /**< @brief Current position in a message buffer */
  unsigned _msgBytesLeft;                         /**< @brief Remaining number of bytes of the message to parse */

  /**
   * Cast the head of the buffer to a TMsg.
   */
  TMsg &GetMessage() {
    return *(TMsg *)(void *)_msgBuffer;
  }

  bool Check(const TMsg *msg, IMIDWORD size) const;

public:
  void Reset();
  const TMsg *Parse(const IMIBYTE buffer[], IMIDWORD size);
};


/**
 * @brief Resets the state of the parser
 */
void CDevIMI::CMsgParser::Reset()
{
  _msgBytesLeft = 0;
  _msgBufferPos = 0;
  _state = STATE_NOT_SYNC;
}


/**
 * @brief Verifies received message
 *
 * @param msg Message to check
 * @param size Size of received message
 *
 * @return Verification status
 */
bool CDevIMI::CMsgParser::Check(const TMsg *msg, IMIDWORD size) const
{
  // minimal size of comm message
  if(size < IMICOMM_MSG_HEADER_SIZE + IMICOMM_CRC_LEN)
    return false;

  // check signature
  if(msg->syncChar1 != IMICOMM_SYNC_CHAR1 || msg->syncChar2 != IMICOMM_SYNC_CHAR2)
    return false;

  // check size
  if(msg->payloadSize != size - IMICOMM_MSG_HEADER_SIZE - IMICOMM_CRC_LEN)
    return false;

  // check CRC
  IMIWORD crc1 = CDevIMI::CRC16Checksum(((IMIBYTE*)msg) + IMICOMM_SYNC_LEN, IMICOMM_MSG_HEADER_SIZE + msg->payloadSize - IMICOMM_SYNC_LEN);
  IMIWORD crc2 = (IMIWORD)(((IMIBYTE*)msg)[size - 1]) | ((IMIWORD)(((IMIBYTE*)msg)[size - 2]) << 8);
  if(crc1 != crc2)
    return false;

  return true;
}


/**
 * @brief Parses received message chunk
 *
 * @param buffer Buffer with received data
 * @param size The size of received data
 *
 * @return Received message or 0 if invalid on incomplete.
 */
const CDevIMI::TMsg *CDevIMI::CMsgParser::Parse(const IMIBYTE buffer[], IMIDWORD size)
{
  const IMIBYTE *ptr = buffer;
  const TMsg *msg = 0;

  for(;size; size--) {
    IMIBYTE byte = *ptr++;

    if(_state == STATE_NOT_SYNC) {
      // verify synchronization chars
      if(byte == IMICOMM_SYNC_CHAR1 && _msgBufferPos == 0) {
        _msgBuffer[_msgBufferPos++] = byte;
      }
      else if(byte == IMICOMM_SYNC_CHAR2 && _msgBufferPos == 1) {
        _msgBuffer[_msgBufferPos++] = byte;
        _state = STATE_COMM_MSG;
      }
      else {
        _msgBufferPos = 0;
      }
    }
    else if(_state == STATE_COMM_MSG) {
      if(_msgBufferPos < IMICOMM_MSG_HEADER_SIZE) {
        // copy header
        _msgBuffer[_msgBufferPos++] = byte;
      }
      else {
        if(_msgBufferPos == IMICOMM_MSG_HEADER_SIZE) {
          // verify payload size
          _msgBytesLeft = GetMessage().payloadSize + IMICOMM_CRC_LEN;
          if(_msgBytesLeft > COMM_MAX_PAYLOAD_SIZE + IMICOMM_CRC_LEN) {
            // Invalid length
            Reset();
            continue;
          }
        }

        // copy payload
        _msgBytesLeft--;
        if(_msgBufferPos < sizeof(_msgBuffer)) // Just in case
          _msgBuffer[_msgBufferPos++] = byte;

        if(_msgBytesLeft == 0) {
          // end of message
          if(Check(&GetMessage(), _msgBufferPos))
            msg = &GetMessage();

          // prepare parser for the next message
          Reset();
        }
      }
    }
  }

  return msg;
}



/* *********************** I M I    D E V I C E ************************** */

bool CDevIMI::_connected;
CDevIMI::CMsgParser CDevIMI::_parser;
CDevIMI::TDeviceInfo CDevIMI::_info;
CDevIMI::IMIWORD CDevIMI::_serialNumber;


/**
 * @brief Calculates IMI CRC value
 *
 * @param message Message for which CRC should be provided
 * @param bytes The size of the message
 *
 * @return IMI CRC value
 */
CDevIMI::IMIWORD CDevIMI::CRC16Checksum(const void *message, unsigned bytes)
{
  const IMIBYTE *pData = (const IMIBYTE *)message;

  IMIWORD crc = 0xFFFF;
  for(;bytes; bytes--) {
    crc  = (IMIBYTE)(crc >> 8) | (crc << 8);
    crc ^= *pData++;
    crc ^= (IMIBYTE)(crc & 0xff) >> 4;
    crc ^= (crc << 8) << 4;
    crc ^= ((crc & 0xff) << 4) << 1;
  }

  if (crc == 0xFFFF)
    crc = 0xAAAA;

  return crc;
}


/**
 * @brief Coordinates converter helper
 */
struct CDevIMI::TAngle
{
  union {
    struct {
      IMIDWORD milliminutes:16;
      IMIDWORD degrees:8;
      IMIDWORD sign:1;
    };
    IMIDWORD value;
  };
};


/**
 * @brief Sets data in IMI Waypoint structure
 *
 * @param decl LK task declaration
 * @param imiIdx The index of IMI waypoint to set
 * @param imiWp IMI waypoint structure to set
 */
void CDevIMI::IMIWaypoint(const Declaration &decl, unsigned imiIdx, TWaypoint &imiWp)
{
  unsigned idx = imiIdx == 0 ? 0 :
    (imiIdx == (unsigned)decl.size() + 1 ? imiIdx - 2 : imiIdx - 1);
  const Declaration::TurnPoint &tp = decl.TurnPoints[idx];
  const Waypoint &wp = tp.waypoint;

  // set name
  unicode2usascii(wp.Name.c_str(), imiWp.name, sizeof(imiWp.name));

  // set latitude
  TAngle a;
  double angle = wp.Location.Latitude.value_degrees();
  if((a.sign = (angle < 0) ? 1 : 0) != 0)
    angle *= -1;
  a.degrees = static_cast<IMIDWORD>(angle);
  a.milliminutes = static_cast<IMIDWORD>((angle - a.degrees) * 60 * 1000);
  imiWp.lat = a.value;

  // set longitude
  angle = wp.Location.Longitude.value_degrees();
  if((a.sign = (angle < 0) ? 1 : 0) != 0)
    angle *= -1;
  a.degrees = static_cast<IMIDWORD>(angle);
  a.milliminutes = static_cast<IMIDWORD>((angle - a.degrees) * 60 * 1000);
  imiWp.lon = a.value;

  // TAKEOFF and LANDING do not have OZs
  if(imiIdx == 0 || imiIdx == (unsigned)decl.size() + 1)
    return;

  // set observation zones
  if(imiIdx == 1) {
    // START
    imiWp.oz.style = 3;
    switch(tp.shape) {
    case Declaration::TurnPoint::CYLINDER: // cylinder
      imiWp.oz.A1 = 1800;
      break;
    case Declaration::TurnPoint::LINE: // line
      imiWp.oz.line_only = 1;
      break;
    case Declaration::TurnPoint::SECTOR: // fai sector
      imiWp.oz.A1 = 450;
      break;
    }
    imiWp.oz.R1 = (IMIDWORD)std::min(250000u, tp.radius);
  } else if(imiIdx == (unsigned)decl.size()) {
    // FINISH
    imiWp.oz.style = 4;
    switch(tp.shape) {
    case Declaration::TurnPoint::CYLINDER: // cylinder
      imiWp.oz.A1 = 1800;
      break;
    case Declaration::TurnPoint::LINE: // line
      imiWp.oz.line_only = 1;
      break;
    case Declaration::TurnPoint::SECTOR: // fai sector
      imiWp.oz.A1 = 450;
      break;
    }
    imiWp.oz.R1 = (IMIDWORD)std::min(250000u, tp.radius);
  } else {
    // TPs
    imiWp.oz.style = 2;
    switch(tp.shape) {
    case Declaration::TurnPoint::CYLINDER: // cylinder
      imiWp.oz.A1 = 1800;
      imiWp.oz.R1 = (IMIDWORD)std::min(250000u, tp.radius);
      break;
    case Declaration::TurnPoint::SECTOR: // sector
      imiWp.oz.A1 = 450;
      imiWp.oz.R1 = (IMIDWORD)std::min(250000u, tp.radius);
      break;
    case Declaration::TurnPoint::LINE: // line
      assert(0);
      break;
    }
  }

  // other unused data
  imiWp.oz.maxAlt = 0;
  imiWp.oz.reduce = 0;
  imiWp.oz.move   = 0;
}


/**
 * @brief Sends message buffer to a device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 * @param msg IMI message to send
 *
 * @return Operation status
 */
bool CDevIMI::Send(Port &d, const TMsg &msg)
{
  d.Write(&msg, IMICOMM_MSG_HEADER_SIZE + msg.payloadSize + 2);
  return true;
}


/**
 * @brief Prepares and sends the message to a device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 * @param msgID ID of the message to send
 * @param payload Payload buffer to use for the message
 * @param payloadSize The size of the payload buffer
 * @param parameter1 1st parameter for to put in the message
 * @param parameter2 2nd parameter for to put in the message
 * @param parameter3 3rd parameter for to put in the message
 *
 * @return Operation status
 */
bool CDevIMI::Send(Port &d,
                   IMIBYTE msgID, const void *payload /* =0 */, IMIWORD payloadSize /* =0 */,
                   IMIBYTE parameter1 /* =0 */, IMIWORD parameter2 /* =0 */, IMIWORD parameter3 /* =0 */)
{
  if(payloadSize > COMM_MAX_PAYLOAD_SIZE)
    return false;

  TMsg msg;
  memset(&msg, 0, sizeof(msg));

  msg.syncChar1 = IMICOMM_SYNC_CHAR1;
  msg.syncChar2 = IMICOMM_SYNC_CHAR2;
  msg.sn = _serialNumber;
  msg.msgID = msgID;
  msg.parameter1 = parameter1;
  msg.parameter2 = parameter2;
  msg.parameter3 = parameter3;
  msg.payloadSize = payloadSize;
  memcpy(msg.payload, payload, payloadSize);

  IMIWORD crc = CRC16Checksum(((IMIBYTE*)&msg) + 2, payloadSize + IMICOMM_MSG_HEADER_SIZE - 2);
  msg.payload[payloadSize] = (IMIBYTE)(crc >> 8);
  msg.payload[payloadSize + 1] = (IMIBYTE)crc;

  return Send(d, msg);
}


/**
 * @brief Receives a message from the device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 * @param extraTimeout Additional timeout to wait for the message
 * @param expectedPayloadSize Expected size of the message
 *
 * @return Pointer to a message structure if expected message was received or 0 otherwise
 */
const CDevIMI::TMsg *CDevIMI::Receive(Port &d, unsigned extraTimeout,
                                      unsigned expectedPayloadSize)
{
  if(expectedPayloadSize > COMM_MAX_PAYLOAD_SIZE)
    expectedPayloadSize = COMM_MAX_PAYLOAD_SIZE;

  // set timeout
  unsigned baudrate = d.GetBaudrate();
  if (!baudrate)
    return NULL;

  unsigned timeout = extraTimeout + 10000 * (expectedPayloadSize + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10) / baudrate;
  if(!d.SetRxTimeout(timeout))
    return NULL;

  // wait for the message
  const TMsg *msg = 0;
  timeout += MonotonicClockMS();
  while(MonotonicClockMS() < timeout) {
    // read message
    IMIBYTE buffer[64];
    IMIDWORD bytesRead = d.Read(buffer, sizeof(buffer));
    if(bytesRead == 0)
      continue;

    // parse message
    const TMsg *lastMsg = _parser.Parse(buffer, bytesRead);
    if(lastMsg) {
      // message received
      if(lastMsg->msgID == MSG_ACK_NOTCONFIG)
        Disconnect(d);
      else if(lastMsg->msgID != MSG_CFG_KEEPCONFIG)
        msg = lastMsg;

      break;
    }
  }

  // restore timeout
  if(!d.SetRxTimeout(0))
    return NULL;

  return msg;
}


/**
 * @brief Sends a message and waits for a confirmation from the device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 * @param msgID ID of the message to send
 * @param payload Payload buffer to use for the message
 * @param payloadSize The size of the payload buffer
 * @param reMsgID Expected ID of the message to receive
 * @param retPayloadSize Expected size of the received message
 * @param parameter1 1st parameter for to put in the message
 * @param parameter2 2nd parameter for to put in the message
 * @param parameter3 3rd parameter for to put in the message
 * @param extraTimeout Additional timeout to wait for the message
 * @param retry Number of send retries
 *
 * @return Pointer to a message structure if expected message was received or 0 otherwise
 */
const CDevIMI::TMsg *CDevIMI::SendRet(Port &d,
                                      IMIBYTE msgID, const void *payload, IMIWORD payloadSize,
                                      IMIBYTE reMsgID, IMIWORD retPayloadSize,
                                      IMIBYTE parameter1 /* =0 */, IMIWORD parameter2 /* =0 */, IMIWORD parameter3 /* =0 */,
                                      unsigned extraTimeout /* =300 */, int retry /* =4 */)
{
  unsigned baudRate = d.GetBaudrate();
  if (!baudRate)
    return NULL;

  extraTimeout += 10000 * (payloadSize + sizeof(IMICOMM_MSG_HEADER_SIZE) + 10) / baudRate;
  while (retry--) {
    if(Send(d, msgID, payload, payloadSize, parameter1, parameter2, parameter3)) {
      const TMsg *msg = Receive(d, extraTimeout, retPayloadSize);
      if(msg && msg->msgID == reMsgID && (retPayloadSize == (IMIWORD)-1 || msg->payloadSize == retPayloadSize))
        return msg;
    }
  }

  return NULL;
}



/**
 * @brief Connects to the device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 *
 * @return Operation status
 */
bool CDevIMI::Connect(Port &d)
{
  if(_connected)
    if(!Disconnect(d))
      return false;

  _connected = false;
  memset(&_info, 0, sizeof(_info));
  _serialNumber = 0;
  _parser.Reset();

  // check connectivity
  if(Send(d, MSG_CFG_HELLO)) {
    const TMsg *msg = Receive(d, 100, 0);
    if (!msg || msg->msgID != MSG_CFG_HELLO)
      return false;

    _serialNumber = msg->sn;
  }

  // configure baudrate
  unsigned baudRate = d.GetBaudrate();
  if (!baudRate)
    return false;

  if(!Send(d, MSG_CFG_STARTCONFIG, 0, 0, IMICOMM_BIGPARAM1(baudRate), IMICOMM_BIGPARAM2(baudRate)))
    return false;

  // get device info
  for(int i = 0; i < 4; i++) {
    if(Send(d, MSG_CFG_DEVICEINFO)) {
      const TMsg *msg = Receive(d, 300, sizeof(TDeviceInfo));
      if(msg) {
        if(msg->msgID == MSG_CFG_DEVICEINFO) {
          if(msg->payloadSize == sizeof(TDeviceInfo)) {
            memcpy(&_info, msg->payload, sizeof(TDeviceInfo));
          } else if(msg->payloadSize == 16) {
            // old version of the structure
            memset(&_info, 0, sizeof(TDeviceInfo));
            memcpy(&_info, msg->payload, 16);
          }
          _connected = true;
          return true;
        }
      } else {
        return false;
      }
    }
  }

  return false;
}


/**
 * @brief Sends task declaration
 *
 * @param d Device handle
 * @param decl Task declaration data
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 *
 * @return Operation status
 */
bool CDevIMI::DeclarationWrite(Port &d, const Declaration &decl)
{
  if (!_connected)
    return false;

  TDeclaration imiDecl;
  memset(&imiDecl, 0, sizeof(imiDecl));

  // idecl.date ignored - will be set by FR
  unicode2usascii(decl.PilotName,        imiDecl.header.plt, sizeof(imiDecl.header.plt));
  // decl.header.db1Year = year; decl.header.db1Month = month; decl.header.db1Day = day;
  unicode2usascii(decl.AircraftType,     imiDecl.header.gty, sizeof(imiDecl.header.gty));
  unicode2usascii(decl.AircraftReg,     imiDecl.header.gid, sizeof(imiDecl.header.gid));
  unicode2usascii(decl.CompetitionId,    imiDecl.header.cid, sizeof(imiDecl.header.cid));
  unicode2usascii(_T(""), imiDecl.header.ccl, sizeof(imiDecl.header.ccl));
  // strncpy(decl.header.clb, idecl.clb, sizeof(decl.header.clb));
  // strncpy(decl.header.sit, idecl.sit, sizeof(decl.header.sit));
  // strncpy(decl.header.cm2, idecl.cm2, sizeof(decl.header.cm2));
  // decl.header.db2Year = year; decl.header.db2Month = month; decl.header.db2Day = day;
  unicode2usascii(_T("XCSOARTASK"), imiDecl.header.tskName, sizeof(imiDecl.header.tskName));
  // decl.header.tskYear = year; decl.header.tskMonth = month; decl.header.tskDay = day;
  // decl.header.tskNumber = MIN(9999, idecl.tskNumber);

  IMIWaypoint(decl, 0, imiDecl.wp[0]);
  for (unsigned i = 0; i < decl.size(); i++)
    IMIWaypoint(decl, i + 1, imiDecl.wp[i + 1]);
  IMIWaypoint(decl, decl.size() + 1, imiDecl.wp[decl.size() + 1]);

  // send declaration for current task
  const TMsg *msg = SendRet(d, MSG_DECLARATION, &imiDecl, sizeof(imiDecl), MSG_ACK_SUCCESS, 0, -1);

  return msg;
}


/**
 * @brief Disconnects from the device
 *
 * @param d Device handle
 * @param errBufSize The size of the buffer for error string
 * @param errBuf The buffer for error string
 *
 * @return Operation status
 */
bool CDevIMI::Disconnect(Port &d)
{
  if(_connected) {
    if(Send(d, MSG_CFG_BYE)) {
      _connected = false;
      memset(&_info, 0, sizeof(_info));
      _serialNumber = 0;
      return true;
    }
  }
  return false;
}

bool
CDevIMI::DeclareTask(Port &d, const Declaration &declaration)
{
  // verify WP number
  if (declaration.size() < 2 || declaration.size() > 13)
    return false;

  // stop Rx thread
  if(!d.StopRxThread())
    return false;

  // set new Rx timeout
  bool status = d.SetRxTimeout(2000);
  if(status) {
    // connect to the device
    status = Connect(d);
    if(status) {
      // task declaration
      status = status && DeclarationWrite(d, declaration);
    }

    // disconnect
    status = Disconnect(d) && status;

    // restore Rx timeout (we must try that always; don't overwrite error descr)
    status = d.SetRxTimeout(0) && status;
  }

  // restart Rx thread
  status = d.StartRxThread() && status;

  return status;
}



void CDevIMI::Register()
{
  _connected = false;
  memset(&_info, 0, sizeof(_info));
  _serialNumber = 0;
}



class IMIDevice : public AbstractDevice {
private:
  Port *port;

public:
  IMIDevice(Port *_port):port(_port) {
    CDevIMI::Register();
  }

  virtual bool Declare(const Declaration &declaration,
                       OperationEnvironment &env);
};

bool
IMIDevice::Declare(const Declaration &declaration,
                   OperationEnvironment &env)
{
  if (port == NULL)
    return false;

  return CDevIMI::DeclareTask(*port, declaration);
}

static Device *
IMICreateOnPort(Port *com_port)
{
  return new IMIDevice(com_port);
}

const struct DeviceRegister imi_device_driver = {
  _T("IMI ERIXX"),
  DeviceRegister::DECLARE,
  IMICreateOnPort,
};
