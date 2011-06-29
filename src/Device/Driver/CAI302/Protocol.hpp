/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Compiler.h"

#define CtrlC 0x03

#define swap(x) x = ((((x<<8) & 0xff00) | ((x>>8) & 0x00ff)) & 0xffff)

#pragma pack(push, 1) // force byte alignment

/** Structure for CAI302 device info */
struct cai302_Wdata_t {
  unsigned char result[3];
  unsigned char reserved[15];
  unsigned char ID[3];
  unsigned char Type;
  unsigned char Version[5];
  unsigned char reserved2[5];
  unsigned char cai302ID;
  unsigned char reserved3[2];
} gcc_packed;

/** Structure for CAI302 Odata info */
struct cai302_OdataNoArgs_t {
  unsigned char result[3];
  unsigned char PilotCount;
  unsigned char PilotRecordSize;
} gcc_packed;

/** Structure for CAI302 settings */
struct cai302_OdataPilot_t {
  unsigned char  result[3];
  char           PilotName[24];
  unsigned char  OldUnit; // old unit
  unsigned char  OldTemperaturUnit; // 0 = Celcius, 1 = Farenheight
  unsigned char  SinkTone;
  unsigned char  TotalEnergyFinalGlide;
  unsigned char  ShowFinalGlideAltitude;
  unsigned char  MapDatum; // ignored on IGC version
  unsigned short ApproachRadius;
  unsigned short ArrivalRadius;
  unsigned short EnrouteLoggingInterval;
  unsigned short CloseTpLoggingInterval;
  unsigned short TimeBetweenFlightLogs; // [Minutes]
  unsigned short MinimumSpeedToForceFlightLogging; // (Knots)
  unsigned char  StfDeadBand; // (10ths M/S)
  unsigned char  ReservedVario; // multiplexed w/ vario mode:
                                // Tot Energy, SuperNetto, Netto
  unsigned short UnitWord;
  unsigned short Reserved2;
  unsigned short MarginHeight; // (10ths of Meters)
  unsigned char  Spare[60]; // 302 expect more data than the documented filed
                            // be shure there is space to hold the data
} gcc_packed;

/** Structure for CAI302 glider response */
struct cai302_GdataNoArgs_t {
  unsigned char result[3];
  unsigned char GliderRecordSize;
} gcc_packed;

/** Structure for CAI302 glider data */
struct cai302_Gdata_t {
  unsigned char  result[3];
  unsigned char  GliderType[12];
  unsigned char  GliderID[12];
  unsigned char  bestLD;
  unsigned char  BestGlideSpeed;
  unsigned char  TwoMeterSinkAtSpeed;
  unsigned char  Reserved1;
  unsigned short WeightInLiters;
  unsigned short BallastCapacity;
  unsigned short Reserved2;
  unsigned short ConfigWord; // locked(1) = FF FE.  unlocked(0) = FF FF
  unsigned short WingArea; // 100ths square meters
  unsigned char  Spare[60]; // 302 expect more data than the documented filed
                            // be shure there is space to hold the data
} gcc_packed;

#pragma pack(pop)

#endif
