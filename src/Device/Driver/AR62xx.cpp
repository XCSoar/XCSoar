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

/*
 * The driver is derived from the KRT2-driver. This version has implemented two methods yet:
 * setting the active frequency
 * setting the passive frequency
 * It can be added:
 * setting/reading dual scan on/off
 * reading frequency change on the radio
 * setting/reading squelsh settings
 *
 */

#include "Device/Driver/AR62xx.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "Thread/Cond.hxx"
#include "Thread/Mutex.hpp"
#include "Util/CharUtil.hxx"
#include "Util/StaticFifoBuffer.hxx"
#include "LogFile.hpp"

#include <stdint.h>
#include <stdio.h>

//Einige Konstanten und Ausdruecke, das AR62xx hat ein Binaerprotokoll
constexpr uint8_t HEADER_ID = 0xA5;
#define PROTID  0x14;
#define QUERY   BIT(7)
#define DUAL    BIT(8)
#define SQUELCH BIT(7)
#define MAX_CMD_LEN 128
#define ACTIVE_STATION  1
#define PASSIVE_STATION 0
#define RoundFreq(a) ((int)((a)*1000.0+0.5)/1000.0)
#define Freq2Idx(a) (int)(((a)-118.0) * 3040/(137.00-118.0)+0.5)
#define BIT(n) (1 << (n))
#define NAME_SIZE 30

typedef union {
  uint16_t intVal16;
  uint8_t  intVal8[2];
} IntConvertStruct;

typedef struct {
  double ActiveFrequency;    //active station frequency
  double PassiveFrequency;   // passive (or standby) station frequency
  TCHAR PassiveName[NAME_SIZE] ; // passive (or standby) station name
  TCHAR ActiveName[NAME_SIZE] ;  //active station name
  int Volume ;               // Radio Volume
  int Squelch ;              // Radio Squelch
  int Vox ;                  // Radio Intercom Volume
  bool Changed;              // Parameter Changed Flag            (TRUE = parameter changed)
  bool Enabled;              // Radio Installed d Flag            (TRUE = Radio found)
  bool Dual;                 // Dual Channel mode active flag     (TRUE = on)
  bool Enabled8_33;          // 8,33kHz Radio enabled             (TRUE = 8,33kHz)
  bool RX;                   // Radio reception active            (TRUE = reception)
  bool TX;                   // Radio transmission active         (TRUE = transmission)
  bool RX_active;            // Radio reception on active station (TRUE = reception)
  bool RX_standy;            // Radio reception on passive        (standby) station
  bool lowBAT;               // Battery low flag                  (TRUE = Batt low)
  bool TXtimeout;            // Timeout while transmission (2Min)
} Radio_t;

IntConvertStruct CRC;
IntConvertStruct sFrequency;
IntConvertStruct sStatus;

volatile bool bSending = false;
//End Einige Konstanten und Ausdruecke, das AR62xx hat ein Binaerprotokoll


/**
 * AR62xx device class.
 *
 * This class provides the interface to communicate with the AR62xx radio.
 * The driver retransmits messages in case of a failure.
 */
class AR62xxDevice final : public AbstractDevice {
  static constexpr unsigned CMD_TIMEOUT = 250; //!< Command timeout in millis.
  static constexpr unsigned NR_RETRIES = 3; //!< Number of tries to send a command.

  static constexpr char STX = 0x02; //!< Command start character.
  static constexpr char ACK = 0x06; //!< Command acknowledged character.
  static constexpr char NAK = 0x15; //!< Command not acknowledged character.
  static constexpr char NO_RSP = 0; //!< No response received yet.

  static constexpr size_t MAX_NAME_LENGTH = 10; //!< Max. radio station name length.
  //! Port the radio is connected to.
  Port &port;
  //! Expected length of the message just receiving.
  size_t expected_msg_length{};
  //! Buffer which receives the messages send from the radio.
  StaticFifoBuffer<uint8_t, 256u> rx_buf;
  //! Last response received from the radio.
  uint8_t response;
  //! Condition to signal that a response was received from the radio.
  Cond rx_cond;
  //! Mutex to be locked to access response.
  Mutex response_mutex;
  // RadioPara aus LK8000
  Radio_t RadioPara;

public:
  /**
   * Constructor of the radio device class.
   *
   * @param _port Port the radio is connected to.
   */
  AR62xxDevice(Port &_port);

private:
  /**
   * Sends a message to the radio.
   *
   * @param msg Message to be send to the radio.
   */
  bool Send(const uint8_t *msg, unsigned msg_size, OperationEnvironment &env);

  /*
   * Called by "PutActiveFrequency"
   * doing the real work
   */
  bool AR620xPutFreqActive(double Freq, const TCHAR *StationName, OperationEnvironment &env);

  /*
   * Called by "PutStandbyFrequency"
   * doing the real work
   */
  bool AR620xPutFreqStandby(double Freq, const TCHAR *StationName, OperationEnvironment &env);

  /*
   * Creates the correct index for a given readable frequency
   */
  uint16_t Frq2Idx(double fFreq);

  /*
   * Creates a correct frequency-number given by the index
   */
  double Idx2Freq(uint16_t uFreqIdx);

  /*
   * This function sets the station name and frequency on the AR62xx
   */
  int SetAR620xStation(uint8_t *Command, int Active_Passive, double fFrequency, const TCHAR* Station);

  /*
   * Parses the messages which XCSoar receives from the radio.
   */
  bool AR620xParseString(const char *String, size_t len);

  int AR620x_Convert_Answer(uint8_t  *szCommand, int len, uint16_t CRC);

  uint8_t toHexString(const void* data, size_t size);

public:
  /**
   * Sets the active frequency on the radio.
   */
  virtual bool PutActiveFrequency(RadioFrequency frequency,
                                  const TCHAR *name,
                                  OperationEnvironment &env) override;
  /**
   * Sets the standby frequency on the radio.
   */
  virtual bool PutStandbyFrequency(RadioFrequency frequency,
                                   const TCHAR *name,
                                   OperationEnvironment &env) override;
  /**
   * Receives and handles data from the radio.
   *
   * The function parses messages send by the radio.
   * Because all control characters (e.g. HEADER_ID, PROTOKOLL_ID, STX, ACK, NAK, ...)
   * can be part of the payload of the messages, it is important
   * to separate the messages to distinguish control characters
   * from payload characters.
   *
   * If a response to a command is received, the function notifies
   * the sender. This could trigger a retransmission in case of a
   * failure.
   */
  virtual bool DataReceived(const void *data, size_t length, struct NMEAInfo &info) override;
};

// Little helpers for me
//size_t = long unsigned int
//unit16_t = unsigned short int
//uint8_t = unsigned char
//TCHAR = char

/*
 * Converting to a hex string for a useful output via LogFormat
 */
uint8_t AR62xxDevice::toHexString(const void* data, size_t size) {
  uint8_t szHex;
  constexpr TCHAR hex_chars[16] = {
          _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'),
          _T('8'), _T('9'), _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F')
  };
  const uint8_t* p = static_cast < const uint8_t* > (data);
  const uint8_t* pend = p + size;
  for(;p < pend; ++p) {
    szHex += (hex_chars[((*p) & 0xF0) >> 4]);
    szHex += (hex_chars[((*p) & 0x0F) >> 0]);
    szHex += ' ';
  }
  return szHex;
}

/*
 * Constructor
 * Port on which the radio is connected
 */
AR62xxDevice::AR62xxDevice(Port &_port) : port(_port) {
  response = ACK;
  RadioPara.Enabled8_33 = true;
}

/**
 * Receives and handles data from the radio.
 *
 * The function parses messages send by the radio.
 * Because all control characters (e.g. HEADER_ID, PROTOKOLL_ID, STX, ACK, NAK, ...)
 * can be part of the payload of the messages, it is important
 * to separate the messages to distinguish control characters
 * from payload characters.
 *
 * If a response to a command is received, the function notifies
 * the sender. This could trigger a retransmission in case of a
 * failure.
 *
 * The initial frequency settings of the radio a delivered by this method and stored in the data struct "info"
 * every time connection is established
 */
bool AR62xxDevice::DataReceived(const void *_data, size_t length, struct NMEAInfo &info) {
  assert(_data != nullptr);
  assert(length > 0);
  const uint8_t *data = (const uint8_t *)_data;
  bool doneok = AR620xParseString((const char *)data, length);
  info.alive.Update(info.clock);
  return doneok;
}

/*
 * Writes the message to the serial port on which the radio is connected
 */
bool AR62xxDevice::Send(const uint8_t *msg, unsigned msg_size, OperationEnvironment &env) {
  //! Number of tries to send a message
  unsigned retries = NR_RETRIES; // i.e. 3 retries
  assert(msg_size > 0);
  do {
    bSending = true;
    response_mutex.Lock();
    response = NO_RSP;
    response_mutex.Unlock();
    // Send the message
    if (!port.FullWrite(msg, msg_size, env, CMD_TIMEOUT)){
      bSending = false;
      response = NAK;
    } else {
      response = ACK;
    }
    // Wait for the response
    response_mutex.Lock();
    rx_cond.timed_wait(response_mutex, CMD_TIMEOUT);
    auto _response = response;
    response_mutex.Unlock();
    bSending = false;
    //LogFormat(_T("AR620x AR62xxDevice::Send: gesendet = %d Zeichen= 0x%04X response= 0x%04X"), msg_size, toHexString(msg, msg_size), _response);
    if (_response == ACK)
      // ACK received, finish, all went well
      return true;

    // No ACK received, retry, possibly an error occurred
    LogFormat(_T("AR620x AR62xxDevice::Send: answer = 0x%04X"), _response);
    retries--;
  } while (retries);
  return false;
}

/*
 * Creates a correct frequency-number to show
 */
double AR62xxDevice::Idx2Freq(uint16_t uFreqIdx) {
  double fFreq= 118.000 + (uFreqIdx & 0xFFF0) * (137.000-118.000)/3040.0;
  switch(uFreqIdx & 0xF){
    case 0:  fFreq += 0.000; break;
    case 1:  fFreq += 0.005; break;
    case 2:  fFreq += 0.010; break;
    case 3:  fFreq += 0.015; break;
    case 4:  fFreq += 0.025; break;
    case 5:  fFreq += 0.030; break;
    case 6:  fFreq += 0.035; break;
    case 7:  fFreq += 0.040; break;
    case 8:  fFreq += 0.050; break;
    case 9:  fFreq += 0.055; break;
    case 10: fFreq += 0.060; break;
    case 11: fFreq += 0.065; break;
    case 12: fFreq += 0.075; break;
    case 13: fFreq += 0.080; break;
    case 14: fFreq += 0.085; break;
    case 15: fFreq += 0.090; break;
  }
  return (fFreq);
}

/*
 * Creates the correct index for a given readable frequency
 */
uint16_t AR62xxDevice::Frq2Idx(double fFreq) {
  uint16_t  uFreIdx= Freq2Idx(fFreq);
  uFreIdx &= 0xFFF0;
  uint8_t uiFrac = ((int)(fFreq*1000.0+0.5)) - (((int)(fFreq *10.0))*100);
  switch(uiFrac) {
    case 0:   uFreIdx += 0;  break;
    case 5:   uFreIdx += 1;  break;
    case 10:  uFreIdx += 2;  break;
    case 15:  uFreIdx += 3;  break;
    case 25:  uFreIdx += 4;  break;
    case 30:  uFreIdx += 5;  break;
    case 35:  uFreIdx += 6;  break;
    case 40:  uFreIdx += 7;  break;
    case 50:  uFreIdx += 8;  break;
    case 55:  uFreIdx += 9;  break;
    case 60:  uFreIdx += 10; break;
    case 65:  uFreIdx += 11; break;
    case 75:  uFreIdx += 12; break;
    case 80:  uFreIdx += 13; break;
    case 85:  uFreIdx += 14; break;
    case 90:  uFreIdx += 15; break;
    case 100: uFreIdx += 0;  break;
    default:   LogFormat(_T("undefined Frequency!  %u -> %u"),uiFrac, uFreIdx); break;
  }
  return (uFreIdx);
}

/*
 * Creates the binary value for the protocol for the radio
 */
static uint16_t CRCBitwise(uint8_t *data, size_t len) {
  uint16_t crc = 0x0000;
  size_t j;
  int i;
  for (j=len; j>0; j--) {
    crc ^= (uint16_t)(*data++) << 8;
    for (i=0; i<8; i++) {
      if (crc & 0x8000) crc = (crc<<1) ^ 0x8005;
      else crc <<= 1;
    }
  }
  return (crc);
}

/*****************************************************************************
 * This function sets the station name and frequency on the AR62xx
 *
 * Active_Passive Active or passive station switch
 * fFrequency     station frequency
 * Station        station Name string
 *
 * The AR62xx always sends and receives both frequencies. the one which remains unchanged
 * is controlled by the "Active_Passive"-flag
 *
 * Station is not used in the moment, AR62xx does not read it yet
 *
 *****************************************************************************/
int AR62xxDevice::SetAR620xStation(uint8_t *Command, int Active_Passive, double fFrequency, const TCHAR* Station) {
  //LogFormat(_T("In SetAR620xStation: %s, %i, %6.3f, %s"), Command, Active_Passive, fFrequency, Station);
  unsigned int len = 0;
  assert(Station !=NULL);
  assert(Command !=NULL);
  if(Command == NULL ) {
    return false;
  }
  //converting both actual frequencies
  IntConvertStruct ActiveFreqIdx;  ActiveFreqIdx.intVal16  = Frq2Idx(RadioPara.ActiveFrequency);
  IntConvertStruct PassiveFreqIdx; PassiveFreqIdx.intVal16 = Frq2Idx(RadioPara.PassiveFrequency);
  Command [len++] = HEADER_ID ;
  Command [len++] = PROTID ;
  Command [len++] = 5;
  //LogFormat(_T("In SetAR620xStation: len = %d, Command(Header_ID, 0x14, 5) = %s"), len, Command);
  switch (Active_Passive) { //converting the frequency which is to be changed
    case ACTIVE_STATION:
      ActiveFreqIdx.intVal16 = Frq2Idx(fFrequency);
      //LogFormat(_T("In SetAR620xStation: >AF:0x%04X  %7.3f"), ActiveFreqIdx.intVal16, fFrequency);
    break;
    default:
    case PASSIVE_STATION:
      PassiveFreqIdx.intVal16 =  Frq2Idx(fFrequency);
      //LogFormat(_T("In SetAR620xStation: >PF:0x%04X  %7.3f"), PassiveFreqIdx.intVal16, fFrequency);
    break;
  }
  Command [len++] = 22; //setting frequencies -command byte in the protocol of the radio
  Command [len++] = ActiveFreqIdx.intVal8[1];
  Command [len++] = ActiveFreqIdx.intVal8[0];
  Command [len++] = PassiveFreqIdx.intVal8[1];
  Command [len++] = PassiveFreqIdx.intVal8[0];
  CRC.intVal16 =  CRCBitwise(Command, len); //Creating the binary value
  Command [len++] = CRC.intVal8[1];
  Command [len++] = CRC.intVal8[0];
  //LogFormat(_T("In SetAR620xStation: len = %d, Command = 0x%04X"), len, toHexString(Command, len));
  return len;
}

/*
 * Called by "PutActiveFrequency"
 * doing the real work
 */
bool AR62xxDevice::AR620xPutFreqActive(double Freq, const TCHAR* StationName, OperationEnvironment &env) {
  int len;
  uint8_t  szTmp[MAX_CMD_LEN];
  len = SetAR620xStation(szTmp ,ACTIVE_STATION, Freq, StationName);
  bool isSend = Send((uint8_t *) &szTmp, len /*sizeof(szTmp)*/, env);  //len seems to be ok!
  if (isSend){
    LogFormat(_T("AR62xxDevice::AR620xPutFreqActive => send: 0x%04X, len: %d, frequency: %6.3f, station: %s"), toHexString(szTmp, len), len, Freq, StationName);
  } else {
    LogFormat(_T("%s"),"Error while AR62xxDevice::AR620xPutFreqActive ...");
  }
  return isSend;
}

/*
 * Called by "PutStandbyFrequency"
 * doing the real work
 */
bool AR62xxDevice::AR620xPutFreqStandby(double Freq, const TCHAR* StationName, OperationEnvironment &env) {
  int len;
  uint8_t  szTmp[MAX_CMD_LEN] = {};
  len = SetAR620xStation(szTmp ,PASSIVE_STATION, Freq, StationName);
  bool isSend = Send((uint8_t *) &szTmp, len, env);
  if (isSend){
    LogFormat(_T("AR62xxDevice::AR620xPutFreqStandby => send: 0x%04X, len: %d, frequency: %6.3f, station: %s"), toHexString(szTmp, len), len, Freq, StationName);
  } else {
    LogFormat(_T("%s"),"Error while AR62xxDevice::AR620xPutFreqStandby ...");
  }
  return isSend;
}

/*
 * Parses the messages which XCSoar receives from the radio.
 */
bool AR62xxDevice::AR620xParseString(const char *String, size_t len) {
  size_t cnt = 0;
  uint16_t CalCRC = 0;
  static  uint16_t Recbuflen = 0;
  int CommandLength = 0;
  #define REC_BUFSIZE 127
  static uint8_t Command[REC_BUFSIZE];

  if(String == NULL) return 0;
  if(len == 0) return 0;

  while (cnt < len) {
    if((uint8_t)String[cnt] == HEADER_ID) Recbuflen =0;
    if(Recbuflen >= REC_BUFSIZE) Recbuflen =0;
    assert(Recbuflen < REC_BUFSIZE);

    Command[Recbuflen++] = (uint8_t) String[cnt++];
    if(Recbuflen == 2){
      if(!(Command[Recbuflen-1] == 0x14)){
        Recbuflen = 0;
      }
    }

    if(Recbuflen >= 3){
      CommandLength = Command[2];
      if(Recbuflen >= (CommandLength+5) ) {// all received
        //for(int i=0; i < (CommandLength+4);i++) {
        //  LogFormat(_T("AR620x->ParseString %d Cmd: 0x%04X"), i, Command[i]);
        //}
        CRC.intVal8[1] =  Command[CommandLength+3];
        CRC.intVal8[0] =  Command[CommandLength+4];
        CalCRC =CRCBitwise(Command, CommandLength+3);
        if(CalCRC == CRC.intVal16 || CRC.intVal16 == 0) {
          if(!bSending) {
            //LogFormat(_T("AR620x->ParseString  Process Command 0x%04X"), Command[3]);
            AR620x_Convert_Answer(Command, CommandLength+5, CalCRC);
//          } else {
//            LogFormat(_T("AR620x->ParseString  skip Command 0x%04X"), Command[3]);
          }
        } else {
          LogFormat(_T("AR620x->ParseString  CRC check fail! CRC.intVal16= 0x%04X  CalCRC= 0x%04X"), CRC.intVal16, CalCRC);
        }
        Recbuflen = 0;
      }
    }
  }
  return  RadioPara.Changed;
}

/*****************************************************************************
 * this function converts a KRT answer sting to a NMEA answer
 *
 * szAnswer       NMEA Answer
 * Answerlen      number of valid characters in the NMEA answerstring
 * szCommand      AR620x binary code to be converted, representing the state of a function (dual scan, squelsh, act. freq., pass. freq., ...)
 * len            length of the AR620x binary code to be converted
 ****************************************************************************/
int AR62xxDevice::AR620x_Convert_Answer(uint8_t *szCommand, int len, uint16_t CRC){
  //LogFormat(_T("AR620x In Convert_Answer 0x%04X %d 0x%04X switchwert %u"), toHexString(szCommand, len), len, CRC, szCommand[3]);
  if(szCommand == NULL) return 0;
  if(len == 0)          return 0;

  static uint16_t uiLastChannelCRC =0;
  static uint16_t uiVolumeCRC      =0;
  static uint16_t uiVersionCRC     =0;
  static uint16_t uiStatusCRC      =0;
  static uint16_t uiSquelchCRC     =0;
  static uint16_t uiRxStatusCRC    =0;
#ifdef RADIO_VOLTAGE
  static uint16_t uiVoltageCRC     =0;
#endif
  uint32_t ulState;
  int processed=0;

  assert(szCommand !=NULL);

  switch ((unsigned char)(szCommand[3] & 0x7F)){
    case 0:
      if(uiVersionCRC!= CRC)  {
         uiVersionCRC = CRC;
         //LogFormat(_T("AR620x Version %u"), CRC);
       }
    break;
    case 3: //Volume settings
      if(uiVolumeCRC != CRC){
        uiVolumeCRC = CRC;
        RadioPara.Changed = true;
        RadioPara.Volume = (50-(int)szCommand[4])/5;
        //LogFormat(_T("AR620x Volume %u"), RadioPara.Volume);
      }
    break;
    case 4: //Squelsh settings
      if(uiSquelchCRC!= CRC){
        uiSquelchCRC = CRC;
        RadioPara.Changed = true;
        RadioPara.Squelch = (int)(szCommand[4]-6)/2+1;  // 6 + (Squelch-1)*2
        //LogFormat(_T("AR620x Squelch %u"), RadioPara.Squelch);
      }
    break;
    case 12: //Dual scan settings
      if(uiStatusCRC != CRC){
        uiStatusCRC = CRC;
        RadioPara.Changed = true;
        sStatus.intVal8[1] = szCommand[4] ;
        sStatus.intVal8[0] = szCommand[5] ;
        if(sStatus.intVal16 & DUAL)
          RadioPara.Dual = true;
        else
          RadioPara.Dual = false;
        //LogFormat(_T("AR620x Dual %u"), RadioPara.Dual);
      }
    break;
#ifdef RADIO_VOLTAGE
    case 21: // actual current of the radio
      if(uiVoltageCRC != CRC) {
        uiVoltageCRC = CRC;
        GPS_INFO.ExtBatt2_Voltage =   8.5 + szCommand[4] *0.1;
        RadioPara.Changed = true;
        //LogFormat(_T("AR620x Supply Voltage: %4.1fV"), GPS_INFO.ExtBatt2_Voltage);
      }
    break;
#endif
    case 22: //Frequency settings, always for both frequencies (active and passive)
      if(uiLastChannelCRC != CRC) {
        uiLastChannelCRC = CRC;
        RadioPara.Changed = true;
        sFrequency.intVal8[1] = szCommand[4] ;
        sFrequency.intVal8[0] = szCommand[5] ;
        RadioPara.ActiveFrequency =  Idx2Freq(sFrequency.intVal16);
        //LogFormat(_T("AR620x <AF: %u %7.3f"), sFrequency.intVal16, RadioPara.ActiveFrequency);

        sFrequency.intVal8[1] = szCommand[6];
        sFrequency.intVal8[0] = szCommand[7] ;
        RadioPara.PassiveFrequency =  Idx2Freq(sFrequency.intVal16);
        //LogFormat(_T("AR620x <PF: %u %7.3f"), sFrequency.intVal16, RadioPara.PassiveFrequency);
        RadioPara.Changed = true;
      }
    break;
    case 64: // general state information
      if(uiRxStatusCRC != CRC) {
        uiRxStatusCRC = CRC;
        ulState = szCommand[4] << 24 | szCommand[5] << 16 | szCommand[6] << 8 | szCommand[7];
        RadioPara.TX        = ((ulState & (BIT(5)| BIT(6))) > 0) ? true : false;
        RadioPara.RX_active = ((ulState & BIT(7)) > 0)  ? true : false;
        RadioPara.RX_standy = ((ulState & BIT(8)) > 0)  ? true : false;
        RadioPara.RX        = (RadioPara.RX_active ||   RadioPara.RX_standy );
        RadioPara.Changed = true;
        //LogFormat(_T("AR620x ulState = 0x%X"), ulState);
        //if(RadioPara.RX) LogFormat(_T("AR620x Rx..."));
        //if(RadioPara.RX_active) LogFormat(_T("AR620x Rx Active..."));
        //if(RadioPara.RX_standy) LogFormat(_T("AR620x Rx Passive..."));
        //if(RadioPara.TX) LogFormat(_T("AR620x Tx..."));
      }
    break;
    default:
    break;
  }
  return processed;  /* return the number of converted characters */
}

/**
 * Sets the active frequency on the radio.
 */
bool AR62xxDevice::PutActiveFrequency(RadioFrequency frequency,
                               const TCHAR* name,
                               OperationEnvironment &env) {
  unsigned int ufreq = frequency.GetKiloHertz();
  double freq = ufreq / 1000.0;
  bool done = AR620xPutFreqActive(freq, (const TCHAR *)name, env);
  return done;
}

/**
 * Sets the standby (passive) frequency on the radio.
 */
bool AR62xxDevice::PutStandbyFrequency(RadioFrequency frequency,
                                const TCHAR *name,
                                OperationEnvironment &env) {
  unsigned int ufreq = frequency.GetKiloHertz();
  double freq = ufreq / 1000.0;
  //LogFormat(_T("In PutStandbyFrequency: %6.3f, %s"), freq, name);
  bool done = AR620xPutFreqStandby(freq, (const TCHAR *)name, env);
  //LogFormat(_T("In PutStandbyFrequency: %6.3f, %i, done..."), freq, done);
  return done;
}

/*
 * Assign the selected port on Object construction
 */
static Device *AR62xxCreateOnPort(const DeviceConfig &config, Port &comPort) {
  Device *dev = new AR62xxDevice(comPort);
  return dev;
}

/*
 * Driver registration in XCSoar, connect to a serial port
 */
const struct DeviceRegister ar62xx_driver = {
  _T("AR62xx"),
  _T("AR62xx"),
  DeviceRegister::NO_TIMEOUT | DeviceRegister::RAW_GPS_DATA,
  AR62xxCreateOnPort,
};
