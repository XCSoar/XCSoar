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

#ifndef XCSOAR_CAI302_PROTOCOL_HPP
#define XCSOAR_CAI302_PROTOCOL_HPP

#include "OS/ByteOrder.hpp"
#include "Compiler.h"

#include <stdint.h>

class OperationEnvironment;
class Port;
struct GeoPoint;

#define CtrlC 0x03

namespace CAI302 {

#pragma pack(push, 1) // force byte alignment

  struct DateTime {
    uint8_t year, month, day, hour, minute, second;

    bool IsDefined() const {
      return month > 0;
    }
  };

  /**
   * Get flight log list.
   *
   * Command: "B 196+N" (N=0..7 for flights N*8..N*8+7)
   */
  struct FileList {
    struct FileInfo {
      struct DateTime start_utc, end_utc;

      char pilot_name[24];

      bool IsDefined() const {
        return start_utc.IsDefined();
      }
    } gcc_packed;

    /**
     * Number of log files stored in 302.
     */
    uint8_t num_files;

    /**
     * Information about the files.
     */
    FileInfo files[8];
  } gcc_packed;

  /**
   * Download file in ASCII (IGC) format.
   *
   * Command: "B 64+N" (Nth file; N=0 means latest file)
   */
  struct FileASCII {
    /**
     * 'Y': file available; 'N': file not available.
     */
    char result;

    PackedBE16 bytes_per_block;

    /**
     * Approximate number of blocks.
     */
    PackedBE16 num_blocks;
  };

  static_assert(sizeof(FileASCII) == 5, "Wrong size");
  static_assert(alignof(FileASCII) == 1, "Wrong alignment");

  /**
   * Download file in binary format (direct memory dump).
   *
   * Command: "B 256+N" (Nth file; N=0 means latest file)
   */
  struct FileBinary {
    /**
     * 'Y': file available; 'N': file not available.
     */
    char result;

    PackedBE16 bytes_per_block;

    uint8_t num_bytes[3];

    PackedBE16 logging_code;
  };

  static_assert(sizeof(FileBinary) == 8, "Wrong size");
  static_assert(alignof(FileBinary) == 1, "Wrong alignment");

  /**
   * Transfer a block.
   *
   * Command: "B N" (next block) or "B R" (retransfer block)
   */
  struct FileData {
    uint16_t valid_bytes;

    // followed by: uint8_t data[bytes_per_block];
  } gcc_packed;

  /**
   * Transfer the signature in ASCII format.
   *
   * Command: "B S"
   */
  struct FileSignatureASCII {
    uint16_t size;

    char signature[201];
  } gcc_packed;

  /** Structure for CAI302 glider response */
  struct PolarMeta {
    uint8_t record_size;
  } gcc_packed;

  /** Structure for CAI302 glider data */
  struct Polar {
    char glider_type[12];
    char glider_id[12];
    uint8_t best_ld;
    uint8_t best_glide_speed;
    uint8_t two_ms_sink_at_speed;
    uint8_t reserved1;
    uint16_t weight_in_litres;
    uint16_t ballast_capacity;
    uint16_t reserved2;
    uint16_t config_word; // locked(1) = FF FE.  unlocked(0) = FF FF
    uint16_t wing_area; // 100ths square meters
  } gcc_packed;

  /** Structure for CAI302 Odata info */
  struct PilotMeta {
    uint8_t count;
    uint8_t record_size;
  } gcc_packed;

  /** Structure for CAI302 "O A" response */
  struct PilotMetaActive {
    uint8_t count;
    uint8_t record_size;
    uint8_t active_index;
  } gcc_packed;

  /** Structure for CAI302 settings */
  struct Pilot {
    char name[24];
    uint8_t old_units; // old unit
    uint8_t old_temperatur_units; // 0 = Celcius, 1 = Farenheight
    uint8_t sink_tone;
    uint8_t total_energy_final_glide;
    uint8_t show_final_glide_altitude_difference;
    uint8_t map_datum; // ignored on IGC version
    uint16_t approach_radius;
    uint16_t arrival_radius;
    uint16_t enroute_logging_interval;
    uint16_t close_logging_interval;
    uint16_t time_between_flight_logs; // [Minutes]
    uint16_t minimum_speed_to_force_flight_logging; // (Knots)
    uint8_t stf_dead_band; // (10ths M/S)
    uint8_t reserved_vario; // multiplexed w/ vario mode:
    // Tot Energy, SuperNetto, Netto
    uint16_t unit_word;
    uint16_t reserved2;
    uint16_t margin_height; // (10ths of Meters)

    unsigned GetUnitBits(unsigned bit, unsigned n) const {
      return (FromBE16(unit_word) >> bit) & (0xffff >> (16 - n));
    }

    void SetUnitBits(unsigned bit, unsigned n, unsigned value) {
      unsigned mask = (0xffff >> (16 - n)) << bit;
      unit_word = ToBE16((FromBE16(unit_word) & ~mask) | (value << bit));
    }

    unsigned GetVarioUnit() const {
      return GetUnitBits(0, 1);
    }

    void SetVarioUnit(unsigned unit) {
      SetUnitBits(0, 1, unit);
    }

    unsigned GetAltitudeUnit() const {
      return GetUnitBits(1, 1);
    }

    void SetAltitudeUnit(unsigned unit) {
      SetUnitBits(1, 1, unit);
    }

    unsigned GetTemperatureUnit() const {
      return GetUnitBits(2, 1);
    }

    void SetTemperatureUnit(unsigned unit) {
      SetUnitBits(2, 1, unit);
    }

    unsigned GetPressureUnit() const {
      return GetUnitBits(3, 1);
    }

    void SetPressureUnit(unsigned unit) {
      SetUnitBits(3, 1, unit);
    }

    unsigned GetDistanceUnit() const {
      return GetUnitBits(4, 2);
    }

    void SetDistanceUnit(unsigned unit) {
      SetUnitBits(4, 2, unit);
    }

    unsigned GetSpeedUnit() const {
      return GetUnitBits(6, 2);
    }

    void SetSpeedUnit(unsigned unit) {
      SetUnitBits(6, 2, unit);
    }

    unsigned GetSinkTone() const {
      return sink_tone;
    }

    void SetSinkTone(unsigned v) {
      sink_tone = v;
    }
  } gcc_packed;

  /** Structure for CAI302 device info */
  struct GeneralInfo {
    uint8_t reserved[15];

    /**
     * Base 36 ASCII.
     */
    char id[3];

    /**
     * 'P' = Prototype, 'F' = Production Firmware.
     */
    char type;

    char version[5];

    uint8_t reserved2[5];

    /**
     * Always 3.
     */
    uint8_t cai302_id;

    uint8_t reserved3[2];
  } gcc_packed;

  /**
   * Returned by "C".
   */
  struct NavpointMeta {
    uint16_t count;
    uint8_t record_size;
  };

  /**
   * A waypoint.  This structure is returned by "C <N>".
   */
  struct Navpoint {
    uint32_t latitude, longitude;
    uint16_t elevation;
    uint16_t id;
    uint16_t attribute;
    char name[12];
    char remark[12];
  } gcc_packed;

#pragma pack(pop)

  bool
  WriteString(Port &port, const char *p, OperationEnvironment &env);

  /**
   * Enter "command" mode, but don't wait for the prompt.
   */
  bool
  CommandModeQuick(Port &port);

  /**
   * Enter "command" mode.
   */
  bool
  CommandMode(Port &port, OperationEnvironment &env);

  /**
   * Send a command, but don't wait for the next command prompt.
   */
  bool
  SendCommandQuick(Port &port, const char *cmd, OperationEnvironment &env);

  /**
   * Send a command, and wait for the next command prompt.
   */
  bool
  SendCommand(Port &port, const char *cmd, OperationEnvironment &env,
              unsigned timeout_ms=2000);

  /**
   * Enter "log" mode, but don't wait for the command prompt.
   */
  bool
  LogModeQuick(Port &port, OperationEnvironment &env);

  /**
   * Enter "log" mode.
   */
  bool
  LogMode(Port &port, OperationEnvironment &env);

  /**
   * Enter "upload" mode.
   */
  bool
  UploadMode(Port &port, OperationEnvironment &env);

  /**
   * Receive a "short" reply from the CAI302.
   *
   * @return the number of data bytes received (not including the 3 byte
   * header), -1 on error
   */
  int
  ReadShortReply(Port &port, void *buffer, unsigned max_size,
                 OperationEnvironment &env, unsigned timeout_ms=2000);

  /**
   * Receive a "large" reply from the CAI302.
   *
   * @return the number of data bytes received (not including the 5
   * byte header), -1 on error, -2 if a short reply with just an
   * upload prompt was seen (probably due to transmission error)
   */
  int
  ReadLargeReply(Port &port, void *buffer, unsigned max_size,
                 OperationEnvironment &env, unsigned timeout_ms=8000);

  /**
   * Send an upload command, and read the short response.  CAI302 must
   * be at the upload prompt already.
   *
   * Note: "upload" means that the CAI302 uploads data to XCSoar,
   * i.e. we're actually receiving from the CAI302.
   *
   * @param command a command string that ends with "\r"
   * @return the number of data bytes received (not including the 3 byte
   * header), -1 on error
   */
  int
  UploadShort(Port &port, const char *command,
              void *response, unsigned max_size,
              OperationEnvironment &env, unsigned timeout_ms=2000);

  /**
   * Send an upload command, and read the large response.  CAI302 must
   * be at the upload prompt already.
   *
   * Note: "upload" means that the CAI302 uploads data to XCSoar,
   * i.e. we're actually receiving from the CAI302.
   *
   * @param command a command string that ends with "\r"
   * @return the number of data bytes received (not including the 5 byte
   * header), -1 on error
   */
  int
  UploadLarge(Port &port, const char *command,
              void *response, unsigned max_size,
              OperationEnvironment &env, unsigned timeout_ms=8000);

  bool
  UploadGeneralInfo(Port &port, GeneralInfo &data, OperationEnvironment &env);

  bool
  UploadFileList(Port &port, unsigned i, FileList &data,
                 OperationEnvironment &env);

  bool
  UploadFileASCII(Port &port, unsigned i, FileASCII &data,
                  OperationEnvironment &env);

  bool
  UploadFileBinary(Port &port, unsigned i, FileBinary &data,
                   OperationEnvironment &env);

  int
  UploadFileData(Port &port, bool next, void *data, unsigned length,
                 OperationEnvironment &env);

  bool
  UploadFileSignatureASCII(Port &port, FileSignatureASCII &data,
                           OperationEnvironment &env);

  bool
  UploadPolarMeta(Port &port, PolarMeta &data, OperationEnvironment &env);

  bool
  UploadPolar(Port &port, Polar &data, OperationEnvironment &env);

  bool
  UploadPilotMeta(Port &port, PilotMeta &data, OperationEnvironment &env);

  bool
  UploadPilotMetaActive(Port &port, PilotMetaActive &data,
                        OperationEnvironment &env);

  bool
  UploadPilot(Port &port, unsigned i, Pilot &data, OperationEnvironment &env);

  /**
   * @return the number of pilots returned in the buffer, or -1 on
   * error
   */
  int
  UploadPilotBlock(Port &port, unsigned start, unsigned count,
                   unsigned record_size, void *buffer,
                   OperationEnvironment &env);

  /**
   * Enter "download" mode.
   */
  bool
  DownloadMode(Port &port, OperationEnvironment &env);

  /**
   * Send a command.  CAI302 must be at the download prompt already.
   */
  bool
  DownloadCommand(Port &port, const char *command,
                  OperationEnvironment &env, unsigned timeout_ms=2000);

  bool
  DownloadPilot(Port &port, const Pilot &data, unsigned ordinal,
                OperationEnvironment &env);

  bool
  DownloadPolar(Port &port, const Polar &data, OperationEnvironment &env);

  bool
  UploadNavpointMeta(Port &port, NavpointMeta &data,
                     OperationEnvironment &env);

  bool
  UploadNavpoint(Port &port, unsigned i, Navpoint &data,
                 OperationEnvironment &env);

  bool
  DownloadNavpoint(Port &port, const GeoPoint &location,
                   int altitude, unsigned id,
                   bool turnpoint, bool airfield, bool markpoint,
                   bool landing_point, bool start_point, bool finish_point,
                   bool home_point, bool thermal_point, bool waypoint,
                   bool airspace,
                   const char *name, const char *remark,
                   OperationEnvironment &env);

  bool
  DeclareTP(Port &port, unsigned i, const GeoPoint &location,
            int altitude, const char *name, OperationEnvironment &env);

  bool
  DeclareSave(Port &port, OperationEnvironment &env);

  /**
   * Restart the CAI302 by sending the command "SIF 0 0".
   */
  bool
  Reboot(Port &port, OperationEnvironment &env);

  /**
   * Power off the CAI302 by sending the command "DIE".
   */
  bool
  PowerOff(Port &port, OperationEnvironment &env);

  /**
   * Start logging unconditionally.
   */
  bool
  StartLogging(Port &port, OperationEnvironment &env);

  /**
   * Stop logging unconditionally.
   */
  bool
  StopLogging(Port &port, OperationEnvironment &env);

  /**
   * Set audio volume 0 is loudest, 170 is silent.
   */
  bool
  SetVolume(Port &port, unsigned volume, OperationEnvironment &env);

  /**
   * Erase all waypoints.
   */
  bool
  ClearPoints(Port &port, OperationEnvironment &env);

  /**
   * Erase the pilot name.
   */
  bool
  ClearPilot(Port &port, OperationEnvironment &env);

  /**
   * Erase all log memory.
   */
  bool
  ClearLog(Port &port, OperationEnvironment &env);

  /**
   * Ask the CAI302 to switch the baud rate.  It does not change the
   * baud rate of the specified #Port.
   */
  bool
  SetBaudRate(Port &port, unsigned baud_rate, OperationEnvironment &env);
}

#endif
