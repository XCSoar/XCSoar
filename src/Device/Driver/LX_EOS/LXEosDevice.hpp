// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Declaration.hpp"
#include "Device/Driver.hpp"
#include "Device/Driver/LX_Eos.hpp"
#include "Device/Port/Port.hpp"
#include "Device/SettingsMap.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "NMEA/DeviceInfo.hpp"
#include "NMEA/InputLine.hpp"
#include "thread/Mutex.hxx"
#include "time/BrokenDate.hpp"
#include "util/AllocatedArray.hxx"
#include "util/ByteOrder.hxx"
#include "util/PackedBigEndian.hxx"
#include "util/PackedFloat.hxx"
#include "util/PackedLittleEndian.hxx"

struct EosDeclarationStruct {
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xCA;
  const uint8_t flag = 0;     // Not used
  const PackedLE16 oo_id = 0; // Not used
  char pilot[19];             // "Name Surname"
  char glider[12];            // Polar name
  char reg_num[8];            // Glider registration number
  char cmp_num[4];            // Competition id

  // 0=STANDARD, 1=15-METER, 2=OPEN, 3=18-METER,
  // 4=WORLD, 5=DOUBLE, 6=MOTOR_GL
  uint8_t byClass;
  char observer[10];              // Not used
  const uint8_t gpsdatum = 0;     // Not used
  const uint8_t fix_accuracy = 0; // Not used
  char gps[60];                   // Not used

  /* auto defined */
  const uint8_t flag2 = 0;         // Not used
  const PackedLE32 input_time = 0; // Not used
  const uint8_t di = 0;            // Not used
  const uint8_t mi = 0;            // Not used
  const uint8_t yi = 0;            // Not used
  /* user defined */
  const uint8_t fd = 0;        // Not used
  const uint8_t fm = 0;        // Not used
  const uint8_t fy = 0;        // Not used
  const PackedLE16 taskid = 0; // Not used

  // Number of TP without Takeoff, Start, Finish and Landing.
  char num_of_tp;

  // 1=Turnpoint (also Start and Finish), 2=Landing, 3=Takeoff
  uint8_t prg[12];

  // TP Longitude in degrees multiplied by 60000.0f
  PackedBE32 lon[12];

  // TP Latitude in degrees multiplied by 60000.0f
  PackedBE32 lat[12];

  char name[12][9]; //< TP Name

  std::byte crc;
};
static_assert(sizeof(EosDeclarationStruct) == 352);
static_assert(alignof(EosDeclarationStruct) == 1);

struct EosObsZoneStruct {
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xF4;

  // TP number 1=Start, 2 = TP1, 3=TP2, 4=Finish
  uint8_t uiTpNr;

  // direction 0= Symmetric, 1=Fixed, 2=Next, 3=Previous
  uint8_t uiDirection;

  uint8_t bAutoNext = 1;    // Is this auto next TP or AAT TP
  uint8_t bIsLine = 0;      // Is this line flag
  PackedFloatLE fA1 = 0;    // Angle A1 in radians
  PackedFloatLE fA2 = 0;    // Angle A2 in radians
  PackedFloatLE fA21 = 0;   // Angle A21 in radians
  PackedLE32 uiR1 = 0;      // Radius R1 in meters
  PackedLE32 uiR2 = 0;      // Radius R2 in meters
  PackedFloatLE fElevation; // Turnpoint elevation

  std::byte crc;
};
static_assert(sizeof(EosObsZoneStruct) == 31);
static_assert(alignof(EosObsZoneStruct) == 1);

struct EosClassStruct {
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xD0;
  char name[9]; // Competition class
  std::byte crc;
};
static_assert(sizeof(EosClassStruct) == 12);
static_assert(alignof(EosClassStruct) == 1);

struct EosGetNumOfFlights
{
  // This command has all bytes constant
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xF2;
  const uint8_t crc = 0x1F;
};
static_assert(sizeof(EosGetNumOfFlights) == 3);
static_assert(alignof(EosGetNumOfFlights) == 1);

struct EosNumOfFlightsResponse
{
  const uint8_t ack = 0x06;
  uint8_t number_of_flights;
  std::byte crc;
};
static_assert(sizeof(EosNumOfFlightsResponse) == 3);
static_assert(alignof(EosNumOfFlightsResponse) == 1);

struct EosRequestFlightInfo
{
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xF0;
  uint8_t flight_id; // First is 1 (newest), only one byte in case of Eos
  std::byte crc;     // CRC
};
static_assert(sizeof(EosRequestFlightInfo) == 4);
static_assert(alignof(EosRequestFlightInfo) == 1);

struct EosFlightInfoResponse
{
  const uint8_t ack = 0x06;
  PackedLE16 uiFlightID; // Flight id (not used, index is used for downloading!)
  char acIGCFileName[10]; // IGC file name (not used)
  PackedLE32 uiDate;      // Date (Julian day)
  PackedLE32 uiTakeOff;   // Takeoff time (seconds after midnight)
  PackedLE32 uiLanding;   // Landing time (seconds after midnight)
  char acName[12];        // Pilot name (not used)
  char acSurname[12];     // Pilot surname (not used)
  char acRegNr[8];        // Registration number (not used)
  char acCompId[8];       // Competition ID (not used)
  int8_t iMinGforce; // Minimum G-force (need to be divided by 10), (not used)
  int8_t iMaxGforce; // Maximum G-force (need to be divided by 10), (not used)
  PackedLE16 uiMaxALT; // Maximum altitude (not used)
  PackedLE16 uiMaxIAS; // Maximum indicated air speed (not used)
  uint8_t reserved[16]; // Unused
  uint8_t reserved2[2]; // Unused, undocumented
  PackedLE32 size;      // IGC file size
  std::byte crc;
};
static_assert(sizeof(EosFlightInfoResponse) == 94);
static_assert(alignof(EosFlightInfoResponse) == 1);

struct EosRequestFlightBlock
{
  const uint8_t syn = 0x02;
  const uint8_t cmd = 0xF1;

  /**
   * Flight ID is the index (1 = newest), not the ID from FlightInfo,
   * Only the first byte is used, as the Eos cannot give flight info for more
   * that 255 flights
   */
  PackedLE16 flight_id;

  PackedLE16 block_id; // First is 0
  std::byte crc;       // CRC
};
static_assert(sizeof(EosRequestFlightBlock) == 7);
static_assert(alignof(EosRequestFlightBlock) == 1);

struct EosFlightBlockResponseHeader
{
  const uint8_t ack = 0x06;
  PackedLE16 block_size; // In bytes
  PackedLE16 block_id;   // First is 0
};
static_assert(sizeof(EosFlightBlockResponseHeader) == 5);
static_assert(alignof(EosFlightBlockResponseHeader) == 1);

/**
 * @brief Struct to hold last known settings of the device
 *
 */
struct VarioSettings
{
  float mc = 0;          // Mc Cready in m/s
  float bugs = 0;        // Bugs in percent (lower value = less bugs)
  float bal = 1;         // Glider mass divided by polar reference mass
  bool uptodate = false; // Setting were received from device at least once
  PolarCoefficients device_polar; // Polar coefficients from device
  float device_reference_mass;    // Reference mass of device's polar
  bool device_reference_mass_uptodate = false;
  GlidePolar xcsoar_polar; // Polar coefficients used by XCSoar
};

/**
 * @brief Struct to hold last known altitude offset
 *
 */
struct AltitudeOffset
{
  bool known = false; // alt_offset in known (LXWP3 was received at least once)

  /* Device is considered not having fw bug where alt_offset doesn't get
   * updated in flight Is false unless proven true by device type from LXWP1 */
  bool reliable = false;

  int16_t last_received = 0; // alt_offset from last LXWP3, in feet
  float meters = 0;          // most recent alt_offset converted to meters
};

class LXEosDevice : public AbstractDevice
{
  Port& port;

public:
  explicit LXEosDevice(Port& _port)
    : port(_port)
  {
  }

private:
  VarioSettings vario_settings; // last known settings of the device
  Mutex settings_mutex;

  AltitudeOffset altitude_offset; // last known settings of the device

  bool LXWP0(NMEAInputLine& line, NMEAInfo& info);
  bool LXWP2(NMEAInputLine& line, NMEAInfo& info);
  bool LXWP3(NMEAInputLine& line, NMEAInfo& info);

  /**
   * @brief Decides if the device has reliable alt_offset parameter in the
   * LXWP3 sentence. Eos is all right (tested on fw 1.7), but Era is reported
   * to have a bug where the alt_offset doesn't get updated if QNH is changed
   * in-flight
   *
   * @param device
   * @return true
   * @return false
   */
  static bool HasReliableAltOffset(DeviceInfo device);

  /**
   * @brief Calculates ratio od dry mass to polar reference mass. This is used
   * to convert water ballast overload. The reference mass of the Eos device
   * may be different from the one in XCSoar. Polar coefficients are used to
   * estimate the reference mass of device's polar.
   *
   * @param settings
   */
  static void CalculateDevicePolarReferenceMass(VarioSettings& settings);

  /**
   * @brief Compare coefficients of polars
   *
   * @param polar1
   * @param polar2
   * @return true: coefficients are the same
   * @return false: coefficients are different or at least one polar is invalid
   */
  static bool ComparePolarCoefficients(PolarCoefficients polar1,
                                       PolarCoefficients polar2);

  /**
   * @brief Sends settings from vario_settings struct to device
   *
   * @param env
   * @return true if successful
   * @return false if vario_settings are not uptodate (previous settings are
   * unknown)
   */
  bool SendNewSettings(OperationEnvironment& env);

  /**
   * @brief Fills destination buffer of given length by characters of string
   * followed by spaces and ended by 0x00
   *
   * @param dest destination buffer
   * @param src source buffer (0x00 terminated string)
   * @param len length of destination buffer
   */
  static void CopyStringSpacePadded(char dest[], const char src[],
                                    const size_t len);

  /**
   * @brief Converts coordinate to value used in declaration
   *
   * @param coord coordinate (longitude or latitude)
   * @note value is in milli angle minutes (x60000)
   */
  static PackedBE32 ConvertCoord(Angle coord);

  /**
   * @brief Send declaration (pilot, glider, and waypoints) to Eos
   *
   * @param declaration
   * @param home (unused)
   * @param env
   */
  void SendDeclaration(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env);

  /**
   * @brief
   *
   * @param tp_nr turnpoint number (counted from 1 = Start)
   * @param declaration
   * @param env
   */
  void SendObsZone(uint8_t tp_nr, const Declaration &declaration,
                   OperationEnvironment &env);

  /**
   * @brief Send competition class string (currently an empty string, as there
   * is no class info in the XCSoar declaration)
   *
   * @param declaration
   * @param env
   */
  void SendCompetitionClass(const Declaration &declaration,
                            OperationEnvironment &env);
  /**
   * @brief Transmit data and wait for ACK response, will throw if
   * ACK is not received within timeout duration
   *
   * @param message Data to be transmitted
   * @param env OperationEnvironment
   */
  void WriteAndWaitForACK(const std::span<const std::byte> &message,
                          OperationEnvironment &env);

  const std::byte ACK{0x06};
  const std::chrono::steady_clock::duration communication_timeout =
      std::chrono::milliseconds(3000);

  /**
   * @brief Get the Number Of Flights in logger. Maximum number that can be
   * read is 0xFF
   *
   * @param env
   */
  uint8_t GetNumberOfFlights(OperationEnvironment& env);

  /**
   * @brief Get the Flight Info
   *
   * @param index 1 = newest, maximum is the flight count read by
   * GetNumberOfFlights
   * @param env
   */
  RecordedFlightInfo GetFlightInfo(const uint8_t index,
                                   OperationEnvironment& env);

  /**
   * @brief Get the Flight Log Block
   *
   * @param flight_id Flight ID from FlightInfo
   * @param block_id First is 0
   * @param env
   * @return
   */
  AllocatedArray<std::byte> GetFlightLogBlock(uint16_t flight_id,
                                              uint16_t block_id,
                                              OperationEnvironment& env);

public:
  /* virtual methods from class Device */
  void LinkTimeout() override;
  bool EnableNMEA(OperationEnvironment& env) override;
  bool ParseNMEA(const char* line, struct NMEAInfo& info) override;
  bool PutBugs(double bugs, OperationEnvironment& env) override;
  bool PutMacCready(double mc, OperationEnvironment& env) override;
  bool PutBallast(double fraction,
                  double overload,
                  OperationEnvironment& env) override;
  void OnCalculatedUpdate(const MoreData& basic,
                          const DerivedInfo& calculated) override;
  bool Declare(const Declaration &declaration, const Waypoint *home,
               OperationEnvironment &env) override;
  bool ReadFlightList(RecordedFlightList& flight_list,
                      OperationEnvironment& env) override;
  bool DownloadFlight(const RecordedFlightInfo& flight,
                      Path path,
                      OperationEnvironment& env) override;
};
