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

/**
 * The driver is derived from the KRT2-driver. 
 * This version has implemented two methods yet:
 * setting the active frequency
 * setting the passive frequency
 * But it can be added:
 * setting/reading the volume of the radio 
 * setting/reading dual scan on/off (listening on both channels)
 * being noticed of frequency change on the radio itself
 * setting/reading squelsh settings
 * setting/reading vox settigns of the intercom
 *
 */

#include "Device/Driver/AR62xx.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "thread/Cond.hxx"
#include "thread/Mutex.hxx"
#include "util/CharUtil.hxx"
#include "util/StaticFifoBuffer.hxx"
#include "util/Compiler.h"
#include "LogFile.hpp"

#include <cstdint>

#include <stdio.h>

/*
 * Some constants and expressions
 * Becker AR62xx radios have a binary protocol
 */
constexpr uint8_t HEADER_ID = 0xA5;

/* Some defines */
#define PROTID  0x14;
#define MAX_CMD_LEN 128
#define ACTIVE_STATION  1
#define PASSIVE_STATION 0

union IntConvertStruct {
  uint16_t int_val16;
  uint8_t  int_val8[2];
};

struct Radio {
  double active_frequency; 
  /* active station frequency, MHz(25KHz mode) or channel (8.33KHz mode)
     MHz are represented as floating point and the channel too is shown
     as floating point on the display 
   */
  double passive_frequency;
  /* passive station frequency, MHz(25KHz mode) or channel (8.33KHz mode)
     MHz are represented as floating point and the channel too is shown
     as floating point on the display 
   */
};

/**
 * AR62xx device class.
 *
 * This class provides the interface to communicate with the AR62xx radio.
 * The driver retransmits messages in case of a failure.
 */
class AR62xxDevice final : public AbstractDevice {
  static constexpr auto CMD_TIMEOUT = std::chrono::milliseconds(250); //!< Command timeout 
  static constexpr unsigned NR_RETRIES = 3; //!< No. retries to send command

  static constexpr char STX = 0x02; /* Command start character */
  static constexpr char ACK = 0x06; /* Command acknowledged character */
  static constexpr char NAK = 0x15; /* Command not acknowledged character */
  static constexpr char NO_RSP = 0; /* No response received yet */

  //! Struct containing active and passive frequency (from LK8000)
  Radio radio_para; 
  //! Port the radio is connected to (from KRT2.cpp)
  Port &port; 
  //! Expected message length just receiving (from KRT2.cpp)
  size_t expected_msg_length{};
  //! Buffer for messages from radio (from KRT2.cpp)
  StaticFifoBuffer<uint8_t, 256u> rx_buf;
  //! Last response received from the radio (frome KRT2.cpp)
  uint8_t response;
  //! Condition to signal a response was received from radio (from KRT2.cpp)
  Cond rx_cond; 
  //! Mutex to be locked to access response (from KRT2.cpp)
  Mutex response_mutex;

public:
  /**
   * Constructor of the radio device class.
   *
   * @param _port Port the radio is connected to.
   */
  explicit AR62xxDevice(Port &_port);

private:
	IntConvertStruct crc;
	IntConvertStruct s_frequency;
	bool b_sending = false;

  /**
   * Sends a message to the radio.
   *
   * @param msg Message to be send to the radio.
   * Doing the real work.
   */
  bool 
  Send(const uint8_t *msg, unsigned msg_size, OperationEnvironment &env);

  /*
   * Creates the correct index for a given readable frequency
   */
  int 
  Frq2Idx(double f_freq) noexcept;

  /*
   * Creates a correct frequency-number given by the index
   */
  double 
  Idx2Freq(uint16_t u_freq_idx);

  /*
   * This function sets the station name and frequency on the AR62xx
   */
  int 
  SetAR620xStation(uint8_t *command, int active_passive, 
                   double f_frequency, const TCHAR* station) noexcept;

  /*
   * Parses the messages which XCSoar receives from the radio.
   */
  bool 
  AR620xParseString(const char *msg_string, size_t len);

  /*
   * this function converts a AR62xx answer sting to a NMEA sequence
   */
  int 
  AR620xConvertAnswer(uint8_t *sz_command, int len, uint16_t crc);

  /*
   * Creates a correct frequency-number to show
   */
  uint16_t 
  Freq2Idx(double a) noexcept;

  /*
   * Not unsed in the Moment
   * Used for function detection inside the binary string received
   */  
/*
  uint32_t 
  Bitshift(int n) noexcept;
*/  
  /*
   * Used for creating the binary string to send
   */
  uint16_t 
  CRCBitwise(uint8_t *data, size_t len) noexcept;
  
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
   * Because all control characters (e.g. HEADER_ID, PROTOKOLL_ID, STX, ACK,
   * NAK, ...)
   * can be part of the payload of the messages, it is important
   * to separate the messages to distinguish control characters
   * from payload characters.
   *
   * If a response to a command is received, the function notifies
   * the sender. This could trigger a retransmission in case of a
   * failure.
   */
  virtual bool DataReceived(std::span<const std::byte> s,
                            struct NMEAInfo &info) noexcept override;
};

/*
 * Constructor
 * Port on which the radio is connected
 */
AR62xxDevice::AR62xxDevice(Port &_port) : port(_port)
{
  AR62xxDevice::response = ACK;
}

/**
 * Receives and handles data from the radio.
 *
 * The function parses messages send by the radio.
 * Because all control characters (e.g. HEADER_ID, PROTOKOLL_ID, STX, ACK,
 * NAK, ...)
 * can be part of the payload of the messages, it is important
 * to separate the messages to distinguish control characters
 * from payload characters.
 *
 * If a response to a command is received, the function notifies
 * the sender. This could trigger a retransmission in case of a
 * failure.
 *
 * The initial frequency settings of the radio a delivered by this method
 * and stored in the data struct "info" every time connection is established
 */
bool
AR62xxDevice::DataReceived(std::span<const std::byte> s,
                         struct NMEAInfo &info) noexcept
{
  assert(!s.empty());

  const auto *data = s.data();
  const auto *const end = data + s.size();
  size_t nbytes = std::min(s.size(), size_t(end - data));
  bool done_ok = AR620xParseString((const char *)data, nbytes);
  info.alive.Update(info.clock);
  return done_ok;

}

/**
 * Writes the message to the serial port on which the radio is connected
 */
bool 
AR62xxDevice::Send(const uint8_t *msg, 
                   unsigned msg_size, 
                   OperationEnvironment &env)
{
  //! Number of tries to send a message i.e. 3 retries, taken from KRT2-driver
  unsigned retries = NR_RETRIES; 
  
  assert(msg_size > 0);
  
  do {
    {
      const std::lock_guard<Mutex> lock(response_mutex);
      response = NO_RSP;
    }
    b_sending = true;

    /* Send the message */
    port.FullWrite(msg, msg_size, env, CMD_TIMEOUT);
    response = ACK;

    /* Wait for the response */
    uint8_t _response;
    {
      std::unique_lock<Mutex> lock(response_mutex);
      rx_cond.wait_for(lock, std::chrono::milliseconds(CMD_TIMEOUT));
      _response = response;
    }
    b_sending = false;
    if (_response == ACK) {
      /* ACK received, finish, all went well */
      return true;
    }
    
    /* No ACK received, retry, possibly an error occured */
    retries--;
  } while (retries);
  
  return false;
}

/*
 * Not used in the moment
 * Helper for Bit shifting, AR62xx have a binary protocol
 */
/*
inline uint32_t 
AR62xxDevice::Bitshift(int n) noexcept
{
	return (1 << (n));
}
*/
/*
 * Creates the correct index for a given readable frequency
 * Because the return value can be a channel number or a frequency in MHZ,
 * depending on use in 25Khz or 8.33Khz mode,
 * the developer in LK8000 might have used "index" for that value
 */
inline int 
AR62xxDevice::Frq2Idx(double f_freq) noexcept
{
	return (int)(((f_freq)-118.0) * 3040/(137.00-118.0)+0.5);
}

/*
 * Creates a correct frequency-number to show
 * Because the return value can be a channel number or a frequency in MHZ,
 * depending on use in 25Khz or 8.33Khz mode,
 * the developer in LK8000 might have used "index" for that value
 */
double 
AR62xxDevice::Idx2Freq(uint16_t u_freq_idx)
{
  double f_freq= 118.000 + (u_freq_idx & 0xFFF0) * (137.000-118.000)/3040.0;
  switch(u_freq_idx & 0xF){
    case 0:  f_freq += 0.000; break;
    case 1:  f_freq += 0.005; break;
    case 2:  f_freq += 0.010; break;
    case 3:  f_freq += 0.015; break;
    case 4:  f_freq += 0.025; break;
    case 5:  f_freq += 0.030; break;
    case 6:  f_freq += 0.035; break;
    case 7:  f_freq += 0.040; break;
    case 8:  f_freq += 0.050; break;
    case 9:  f_freq += 0.055; break;
    case 10: f_freq += 0.060; break;
    case 11: f_freq += 0.065; break;
    case 12: f_freq += 0.075; break;
    case 13: f_freq += 0.080; break;
    case 14: f_freq += 0.085; break;
    case 15: f_freq += 0.090; break;
  }
  return (f_freq);
}

/*
 * Creates the correct index for a given readable frequency or channel
 * Because the return value can be a channel number or a frequency in MHZ,
 * depending on use in 25Khz or 8.33Khz mode,
 * the developer in LK8000 might have used "index" for that value
 */
uint16_t 
AR62xxDevice::Freq2Idx(double f_freq) noexcept
{
  uint16_t  u_fre_idx= Frq2Idx(f_freq);
  u_fre_idx &= 0xFFF0;
  uint8_t uiFrac = ((int)(f_freq*1000.0+0.5)) - (((int)(f_freq *10.0))*100);
  
  switch(uiFrac) {
    case 0:   u_fre_idx += 0;  break;
    case 5:   u_fre_idx += 1;  break;
    case 10:  u_fre_idx += 2;  break;
    case 15:  u_fre_idx += 3;  break;
    case 25:  u_fre_idx += 4;  break;
    case 30:  u_fre_idx += 5;  break;
    case 35:  u_fre_idx += 6;  break;
    case 40:  u_fre_idx += 7;  break;
    case 50:  u_fre_idx += 8;  break;
    case 55:  u_fre_idx += 9;  break;
    case 60:  u_fre_idx += 10; break;
    case 65:  u_fre_idx += 11; break;
    case 75:  u_fre_idx += 12; break;
    case 80:  u_fre_idx += 13; break;
    case 85:  u_fre_idx += 14; break;
    case 90:  u_fre_idx += 15; break;
    case 100: u_fre_idx += 0;  break;
    default:  break;
  }
  return (u_fre_idx);
}

/*
 * Creates the binary value for the message for the radio
 */
uint16_t 
AR62xxDevice::CRCBitwise(uint8_t *data,
                         size_t len) noexcept
{
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

/**
 * This function sets the station name and frequency on the AR62xx
 *
 * active_passive Active or passive station switch
 * f_frequency    station frequency
 * station        station name string
 *
 * The AR62xx always sends and receives both frequencies/channels. 
 * The one which remains unchanged is controlled by the "active_passive"-flag
 *
 * station is not used in the moment, AR62xx does not read it yet
 */
int 
AR62xxDevice::SetAR620xStation(uint8_t *command,
                               int active_passive, 
                               double f_frequency,
                               const TCHAR* station) noexcept
{
  unsigned int len = 0;

  assert(station !=NULL);
  assert(command !=NULL);

  if(command == NULL ) {
    return false;
  }

  /* converting both actual frequencies active and passive */
  IntConvertStruct active_freq_idx;  
  active_freq_idx.int_val16  = Freq2Idx(radio_para.active_frequency);
  IntConvertStruct passive_freq_idx; 
  passive_freq_idx.int_val16 = Freq2Idx(radio_para.passive_frequency);
  command [len++] = HEADER_ID ;
  command [len++] = PROTID ;
  command [len++] = 5;

  /* converting the frequency to be changed */
  switch (active_passive) { 
    case ACTIVE_STATION:
      active_freq_idx.int_val16 = Freq2Idx(f_frequency);
      break;
    default:
    case PASSIVE_STATION:
      passive_freq_idx.int_val16 = Freq2Idx(f_frequency);
      break;
  }

  command [len++] = 22; /* setting frequencies-command byte in the protocol */
  command [len++] = active_freq_idx.int_val8[1];
  command [len++] = active_freq_idx.int_val8[0];
  command [len++] = passive_freq_idx.int_val8[1];
  command [len++] = passive_freq_idx.int_val8[0];
  crc.int_val16 =  CRCBitwise(command, len); /* Creating the binary value */
  command [len++] = crc.int_val8[1];
  command [len++] = crc.int_val8[0];
  return len;
}

/*
 * Parses the messages which XCSoar receives from the radio.
 */
bool 
AR62xxDevice::AR620xParseString(const char *msg_string, 
                                size_t len)
{
  size_t cnt = 0;
  uint16_t cal_crc = 0;
  static  uint16_t rec_buf_len = 0;
  int command_length = 0;
  #define REC_BUFSIZE 127
  static uint8_t command[REC_BUFSIZE];

  if(msg_string == NULL) return 0;
  if(len == 0) return 0;

  while (cnt < len) {
    if((uint8_t)msg_string[cnt] == HEADER_ID) rec_buf_len =0;
    if(rec_buf_len >= REC_BUFSIZE) rec_buf_len =0;
    assert(rec_buf_len < REC_BUFSIZE);

    command[rec_buf_len++] = (uint8_t) msg_string[cnt++];
    if(rec_buf_len == 2){
      if(!(command[rec_buf_len-1] == 0x14)){
        rec_buf_len = 0;
      }
    }

    if(rec_buf_len >= 3){
      command_length = command[2];
      if(rec_buf_len >= (command_length+5) ) {// all received
        crc.int_val8[1] =  command[command_length+3];
        crc.int_val8[0] =  command[command_length+4];
        cal_crc =CRCBitwise(command, command_length+3);
        if(cal_crc == crc.int_val16 || crc.int_val16 == 0) {
          if(!b_sending) {
            AR620xConvertAnswer(command, command_length+5, cal_crc);
          }
        }
        rec_buf_len = 0;
      }
    }
  }
  return true;
}

/*
 * This function converts a AR62xx answer sting to a readable number
 *
 * sz_command     AR620x binary code to be converted, representing the state
 *                of a function (act. freq., pass. freq.)
 * len            length of the AR620x binary code to be converted
 */
int 
AR62xxDevice::AR620xConvertAnswer(uint8_t *sz_command, 
                                  int len, 
                                  uint16_t crc)
{
  
  if(sz_command == NULL) return 0;
  if(len == 0) return 0;

  static uint16_t ui_last_channel_crc = 0;
  static uint16_t ui_version_crc = 0;

#ifdef RADIO_VOLTAGE
  static uint16_t ui_voltage_crc = 0;
#endif

  assert(sz_command !=NULL);

  switch ((unsigned char)(sz_command[3] & 0x7F)){
    case 0:
      if(ui_version_crc!= crc)  {
         ui_version_crc = crc;
       }
    break;
#ifdef RADIO_VOLTAGE
    case 21: /* actual voltage of the radio */
      if(ui_voltage_crc != crc) {
        ui_voltage_crc = crc;
        GPS_INFO.ExtBatt2_Voltage = 8.5 + sz_command[4] * 0.1;
      }
    break;
#endif
    case 22: /* Frequency/channel settings, always for active and passive */
      if(ui_last_channel_crc != crc) {
        ui_last_channel_crc = crc;
        s_frequency.int_val8[1] = sz_command[4] ;
        s_frequency.int_val8[0] = sz_command[5] ;
        radio_para.active_frequency = Idx2Freq(s_frequency.int_val16);

        s_frequency.int_val8[1] = sz_command[6];
        s_frequency.int_val8[0] = sz_command[7] ;
        radio_para.passive_frequency = Idx2Freq(s_frequency.int_val16);
      }
    break;
    default:
    break;
  }
  return 0;  /* return the number of converted characters */
}

/**
 * Sets the active frequency on the radio.
 * same as in KRT2.cpp
 */
bool 
AR62xxDevice::PutActiveFrequency(RadioFrequency frequency,
                                 const TCHAR* name, 
                                 OperationEnvironment &env)
{
  unsigned int ufreq = frequency.GetKiloHertz();
  double freq = ufreq / 1000.0;
  int len;
  uint8_t sz_tmp[MAX_CMD_LEN] = {};
  len = SetAR620xStation(sz_tmp ,ACTIVE_STATION, freq, name);
  /* len seems to be the same as sizeof(szTmp)*/
  bool is_sent = Send((uint8_t *) &sz_tmp, len, env);
  return is_sent;
}

/**
 * Sets the standby (passive) frequency on the radio.
 * same as in KRT2.cpp
 */
bool 
AR62xxDevice::PutStandbyFrequency(RadioFrequency frequency,
                                  const TCHAR *name, 
                                  OperationEnvironment &env)
{
  unsigned int ufreq = frequency.GetKiloHertz();
  double freq = ufreq / 1000.0;
  int len;
  uint8_t sz_tmp[MAX_CMD_LEN] = {};
  len = SetAR620xStation(sz_tmp ,PASSIVE_STATION, freq, name);
  bool is_sent = Send((uint8_t *) &sz_tmp, len, env);
  return is_sent;
}

/*
 * Assign the selected port on Object construction
 * same as in KRT2.cpp
 */
static Device *
AR62xxCreateOnPort(const DeviceConfig &config, 
                                  Port &comPort)
{
  Device *dev = new AR62xxDevice(comPort);
  return dev;
}

/*
 * Driver registration in XCSoar, connect to a serial port
 * same as in KRT2.cpp
 */
const struct DeviceRegister ar62xx_driver = {
  _T("AR62xx"),
  _T("AR62xx"),
  DeviceRegister::NO_TIMEOUT | DeviceRegister::RAW_GPS_DATA,
  AR62xxCreateOnPort,
};
