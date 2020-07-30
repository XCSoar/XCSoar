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
 * PREFACE:
 * --------
 * This Device driver reads and sends NMEA-messages of/to a Becker AR/RT 62xx device
 * AR62xx device (VHF transceiver with control-head)
 * RT62xx device (VHF transceiver without or with external control head [RCU62xx])
 * 
 * IMPLEMENTATION INFORMATION:
 * ---------------------------
 * 1) We are permanently reading the broadcasts of the device with active and passive frequency
 *    and saving these frequencies in the variables <active_frequency> and <passive_frequency>
 * 2) We send a NMEA-message to the device containing the active AND passive frequency no matter
 *    if we only want to set a new active/passive frequency.
 *    (if we want to set a new active frequency we send <NEW ACTIVE / OLD PASSIVE>)
 * 
 * Explanation: Why are we always setting both frequencies?
 * --------------------------------------------------------
 * The protocol in general provides two frame-ids (id 1/2) for setting a SINGLE-frequency (active OR passive)
 * but this can't be used for AR6201 due to the fact, that the AR6201 has a control-head (display
 * with controls) which is communicating with the device on NMEA as well.
 * If you try to set a single frequency it will be set in the device, but the control
 * head immediately resets this frequency with the old active/passive frequency.
 * Therefore we have to use frame-id 22 which sets both frequencies and the device broadcasts
 * this change to the control-head. Single frequency-changes will not be broadcasted and therefore
 * the control-head does the reset. This behavior has been confirmed by Becker via mail.
 * 
 * 
 * PROTOCOL DOCUMENTATION:
 * -----------------------
 * This short protocol-structure describes the NMEA-message we receive/send from/to the device
 * for reading/setting frequencies. Of course there is a lot more you can do with this protocol,
 * but not needed by the driver.
 * 
 * ------------------------------------------------------------------------------------------------------
 * This protocol exists to read/write values in AR/RT/RCU 620xx devices
 * 
 * STRUCTURE:
 * ----------
 * <HEADER> <PROTOCOL-ID> <FRAME-LENGTH> <FRAME> <CRC>
 * 
 * Header:
 * -------
 *  is always 0xA5
 *  length: 1 Byte
 * 
 * Protocol-ID:
 * ------------
 *  protocol-identifier for the <frame> contents
 *  The Protocol ID field determines the protocol used for transfer of the package.
 *  Following protocol ID value is defined:
 *  0x14 - 62XX Transceiver Family Data Exchange Protocol
 *  All other values of protocol ID are reserved for different protocols.
 *  length: 1 Byte
 * 
 * Frame-Length:
 * -------------
 *  <FRAME> Field length
 *  length: 1Byte
 * 
 * Frame:
 * ------
 *  Frame structure:
 *  <FRAME-ID> <Value-Block>
 * 
 *  Frame-ID:
 *    22: Active / Passive Channel
 *  
 *  Frame:
 *    Active frequency as ID
 *    Range [0...3039] (118.00 ... 136.990 Mhz) Unit: none (data type: 16-bit unsigned integer)
 *    Passive frequency as ID
 *    Range [0...3039] (118.00 ... 136.990 Mhz) Unit: none (data type: 16-bit unsigned integer)
 *  length: 4 Bytes (Big Endian Order)
 * 
 * CRC:
 * -----
 *  Cyclic redundancy check
 *  length: 2 Bytes (Big Endian Order)
 * 
 */

#include "Device/Driver/AR62xx.hpp"
#include "Device/Driver.hpp"
#include "Device/Port/Port.hpp"
#include "NMEA/Info.hpp"
#include "RadioFrequency.hpp"
#include "OS/ByteOrder.hpp"

#include <cstdint>

/* identifier for active / passive station
 * 1 = active station of AR62xx
 * 0 = passive station of AR62xx */
constexpr unsigned int ACTIVE_STATION = 1;
constexpr unsigned int PASSIVE_STATION = 0;

/* list of possible channels which can be set at each frequency
 * (required for calculation of the frequencyID) */
static const uint16_t CHANNEL_LIST[] = {0, 5, 10, 15, 25, 30, 35, 40, 50, 55, 60, 65, 75, 80, 85, 90};

/* protocol header*/
static const uint8_t PROTOCOL_HEADER = 0xA5;

/* the protocol id determines the protocol used for transfer of the package.
the following protocol id defines the 62XX transceiver family data exchange protocol */
static const uint8_t PROTOCOL_ID = 0x14;

/* The frame-ID to set the active AND passive station
 *  Frame-ID: 22 */
static const uint8_t FRAME_ID = 22;

/* The first frequency which can be set in the device 
 * !!! WITHOUT THE CHANNEL !!!
 * 
 * Explanation:
 * 118000 is the first frequency which can be set in the device
 * 118.0 is the frequency 00 is the channel (if the device is set to 8khz raster)
 * */
static const uint16_t FIRST_FREQUENCY_WITHOUT_CHANNEL = 1180;

/* protocol length to send to AR62xx
 * is 10 as we only need to send/receive 10 bytes
 * when reading / setting frequencies */
static constexpr int PROTOCOL_LENGTH = 10;

class AR62xxDevice final : public AbstractDevice
{

private:
  /* Port the radio is connected to. */
  Port &port;

  /* active station frequency */
  RadioFrequency active_frequency;

  /* passive (standby) station frequency */
  RadioFrequency passive_frequency;

public:
  explicit AR62xxDevice(Port &_port);

private:
  /**
   * @brief Send - Sends a message to the AR62xx
   * 
   * @param msg message
   * @param msg_size size of the message
   * @param env operation environment to run the thread in
   * @return true if message has been sent
   * @return false if message could not be sent
   */
  bool Send(const uint8_t *msg, size_t msg_size, OperationEnvironment &env);

  /**
   * @brief Extracts the active and passive frequency from an NMEA-message and saves current active and passive frequency for later usage
   * 
   * @param string NMEA message
   * @param len length of message
   * @return true if frequency has been saved
   * @return false if frequency could not be saved
   */
  bool SaveActiveAndPassiveFrequency(const uint8_t *string, size_t len);

  /**
   * @brief sets a frequency in the AR62xx
   * 
   * @param frequency the frequency which should be set
   * @param active_passive 1 = active, 2 = passive
   * @param env the environment where the operation runs in
   * @return true frequency has been set
   * @return false frequency could not be set
   */
  bool PutFrequency(RadioFrequency frequency,
                    const int active_passive,
                    OperationEnvironment &env);

public:
  bool PutActiveFrequency(RadioFrequency frequency,
                          const TCHAR *name,
                          OperationEnvironment &env) override;
  bool PutStandbyFrequency(RadioFrequency frequency,
                           const TCHAR *name,
                           OperationEnvironment &env) override;

  bool DataReceived(const void *data, size_t length, struct NMEAInfo &info) override;
};

inline AR62xxDevice::AR62xxDevice(Port &_port) : port(_port) {}

bool AR62xxDevice::DataReceived(const void *_data, size_t length, struct NMEAInfo &info)
{
  /* check if we received data */
  if (length == 0)
  {
    return false;
  }

  /* read the NMEA-message of the device and try to extract the current
   * active and passive frequency for later usage*/
  bool frequency_saved = SaveActiveAndPassiveFrequency((const uint8_t *)_data, length);

  /* send NMEA info if we could read the frequencies */
  if (frequency_saved)
    info.alive.Update(info.clock);

  return frequency_saved;
}

bool AR62xxDevice::Send(const uint8_t *msg, size_t msg_size, OperationEnvironment &env)
{
  return port.FullWrite(msg, msg_size, env, std::chrono::milliseconds(250));
}

/**
 * @brief converts an AR62xx-ID to a real frequency in MHz
 *
 * explanation: the AR62xx uses an integer value for each 8.33/25 kHz frequency starting at 118.000 MHz
 * e.g. frequency-id 0    => 118.000 MHz
 *      frequency-id 1    => 118.005 MHz
 *      frequency-id 3039 => 136.990 MHz
 * @param frequency_id frequency-id
 * @return RadioFrequency frequency in MHz
 */
static RadioFrequency IdToFrequency(const uint16_t frequency_id)
{
  /* the amount of channels which can be set at each frequency */
  unsigned number_of_channels = sizeof(CHANNEL_LIST) / sizeof(*CHANNEL_LIST);

  /* get the channel-id */
  unsigned channel_idx = frequency_id % number_of_channels;

  /* get the channel with the channel-id out of our channel-list */
  unsigned channel = CHANNEL_LIST[channel_idx];

  /* get frequency id */
  unsigned frequency_idx = frequency_id - channel_idx;

  /* calculate frequency offset to the first available frequency*/
  unsigned frequency_offset = frequency_idx / 16;

  /* calculate frequency (without channel)*/
  unsigned frequency = (frequency_offset + FIRST_FREQUENCY_WITHOUT_CHANNEL) * 100;

  /* generate RadioFrequency object */
  RadioFrequency radio_frequency = RadioFrequency();

  /* set frequency including the channel */
  radio_frequency.SetKiloHertz(frequency + channel);

  return radio_frequency;
}

/**
 * @brief converts a frequency to an AR62xx-ID
 * 
 * explanation: the AR62xx uses an integer value for each 8.33/25 kHz frequency starting at 118.000 MHz
 * e.g. frequency-id 0    => 118.000 MHz
 *      frequency-id 1    => 118.005 MHz
 *      frequency-id 3039 => 136.990 MHz
 * 
 * @param frequency frequency in MHz
 * @return uint16_t frequency-id
 */
static uint16_t FrequencyToId(const RadioFrequency freq)
{
  /* the amount of channels which can be set at each frequency */
  unsigned number_of_channels = sizeof(CHANNEL_LIST) / sizeof(*CHANNEL_LIST);

  /* get frequency (first 4 digits (left to right) of the "RadioFrequency") in KiloHertz
   * and convert them to the frequency id
   * 
   * CALCULATION:
   * 1180 = first frequency without decimal and without channel to get the offset to the frequency which should be set
   * 16 = number of channels you can set at one frequency
   * 
   * 1) Frequency / 100 = First 4 Digits of the Radio-Frequency
   * 2) (1) - 1180 = offset to the first frequency available
   * 3) (2) * 16 = frequency-id without channel (channel will be added below)
   */
  uint16_t frequency_id = ((freq.GetKiloHertz() / 100) - FIRST_FREQUENCY_WITHOUT_CHANNEL) * number_of_channels;

  /* get channel out of frequency which is in the last two digits (which will be delivered by mod 100)  */
  uint16_t channel = freq.GetKiloHertz() % 100;

  /* find channel in the list of the 16 available channels and add it's index to frequency-id
   * if the RadioFrequency is invalid (invalid channel) no channel will be added, so we get the plain
   * frequency without the channel */
  for (uint16_t channel_id = 0; channel_id < 16; channel_id++)
  {
    if (channel == CHANNEL_LIST[channel_id])
      frequency_id += channel_id;
  }

  return frequency_id;
}

/**
 * @brief crc algorithm
 * using 16-bit binary polynomial represented by number 0x8005.
 * Bist shall be inserted for CRC calculation starting from the most significant bit of the header byte.
 * The C code below is defined in the Becker protocol definition.
 * 
 * @param data the data to calculate the crc on
 * @param len the length of the data
 * @return uint16_t the crc
 */
static uint16_t CRCBitwise(const uint8_t *p_scr, uint16_t length)
{
  uint16_t crc_val = 0;

  while (length--)
  {
    uint16_t data = *p_scr++ << 8;
    for (uint16_t j = 0; j < 8; j++)
    {
      if ((crc_val ^ data) & 0x8000)
        crc_val = (crc_val << 1) ^ 0x8005;
      else
        crc_val <<= 1;

      data <<= 1;
    }
  }
  return crc_val;
}

bool AR62xxDevice::SaveActiveAndPassiveFrequency(const uint8_t *data, size_t len)
{
  /* the device sends multiple broadcasts in one message which contain
   * information about frequencies, volume, test-status, ...
   * as we are only interested in the message which contains the frequencies we
   * have to search for the messages containing frame-id 22 which is the flag
   * for frequency-information.
   * The frequency-broadcast always has a protocol-length of 10 bytes. so looping
   * gets quite easy.*/
  for (unsigned idx = 0; idx + PROTOCOL_LENGTH <= len; idx++)
  {
    /* check if we received a header which tells us we are receiving frequencies */
    if (data[idx] == PROTOCOL_HEADER && data[idx + 1] == PROTOCOL_ID && data[idx + 3] == FRAME_ID)
    {
      /* calculate crc on the first 8 bytes */
      uint16_t crc_calculated = CRCBitwise(&data[idx], 8);

      /* Byte 8 & 9: crc */
      uint16_t crc_received = ReadUnalignedBE16((const uint16_t *)(const void *)&data[idx + 8]);

      /* check if the crc is correct */
      if (crc_calculated == crc_received)
      {
        /* Byte 4 & 5: active frequency */
        active_frequency = IdToFrequency(ReadUnalignedBE16((const uint16_t *)(const void *)&data[idx + 4]));

        /* Byte 6 & 7: passive frequency */
        passive_frequency = IdToFrequency(ReadUnalignedBE16((const uint16_t *)(const void *)&data[idx + 6]));
        return true;
      }
    }
  }

  /* something was wrong / we got corrupt data / the NMEA-message did not contain the frequency-broadcast */
  return false;
}

bool AR62xxDevice::PutFrequency(RadioFrequency frequency,
                                const int active_passive,
                                OperationEnvironment &env)
{
  /* check if we have saved passive/active frequency. if not we do NOT try to set a
   * frequency as we would overwrite the second frequency (active/passive) which we
   * do not want to set */
  if (!active_frequency.IsDefined() || !passive_frequency.IsDefined())
  {
    return false;
  }

  /* command-buffer (10 bytes needed to create a set-frequency-command) */
  uint8_t command[PROTOCOL_LENGTH];

  /* length of the command */
  unsigned int command_length = 0;

  /* Byte 0: header 
   * (always 0xA5) */
  command[command_length++] = PROTOCOL_HEADER;

  /* Byte 1: protocol-ID 
   * description: protocol identifier ofr the <frame> contents */
  command[command_length++] = PROTOCOL_ID;

  /* Byte 2: frame-length
   * description: frame field length 
   * 
   * is 5 Bytes because of:
   * byte 1 : frame-id
   * byte 2 : first bit of active frequency
   * byte 3 : second bit of active frequency
   * byte 4 : first bit of passive frequency
   * byte 5 : second bit of passive frequency
   * */
  command[command_length++] = 5;

  /* Byte 3: frame-id for setting a frequency
   * 22 =  active/passive frequency   type=16 Bit unsigned Integer (4 Bytes)
  */
  command[command_length++] = FRAME_ID;

  //set active station
  if (active_passive == ACTIVE_STATION)
    active_frequency = frequency;
  else
    passive_frequency = frequency;

  /* Byte 4 & 5: active frequency
   * add frequency bytes (2) */
  WriteUnalignedBE16((uint16_t *)(void *)&command[command_length], FrequencyToId(active_frequency));
  command_length += 2;

  /* Byte 6 & 7: passive frequency
   * add frequency bytes (2) */
  WriteUnalignedBE16((uint16_t *)(void *)&command[command_length], FrequencyToId(passive_frequency));
  command_length += 2;

  /* Byte 8 & 9: crc
   * add crc bytes (2) */
  WriteUnalignedBE16((uint16_t *)(void *)&command[command_length], CRCBitwise(command, command_length));
  command_length += 2;

  /* return if command could be send */
  return Send((uint8_t *)&command, command_length, env);
}

bool AR62xxDevice::PutActiveFrequency(RadioFrequency frequency,
                                      const TCHAR *name,
                                      OperationEnvironment &env)
{
  return PutFrequency(frequency, ACTIVE_STATION, env);
}

bool AR62xxDevice::PutStandbyFrequency(RadioFrequency frequency,
                                       const TCHAR *name,
                                       OperationEnvironment &env)
{
  return PutFrequency(frequency, PASSIVE_STATION, env);
}

static Device *AR62xxCreateOnPort(const DeviceConfig &config, Port &comPort)
{
  return new AR62xxDevice(comPort);
}

const struct DeviceRegister ar62xx_driver = {
    _T("AR62xx"),                 /* Name */
    _T("Becker AR62XX / RT62XX"), /* Display Name */
    DeviceRegister::NO_TIMEOUT | DeviceRegister::RAW_GPS_DATA,
    AR62xxCreateOnPort,
};