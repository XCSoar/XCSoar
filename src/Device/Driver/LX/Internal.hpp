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

#ifndef XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP
#define XCSOAR_DEVICE_DRIVER_LX_INTERNAL_HPP

#include "Device/Driver.hpp"
#include "DateTime.hpp"

class LXDevice: public AbstractDevice
{
  Port *port;
  BrokenDate DeclDate;

public:
  LXDevice(Port *_port)
    :port(_port),
     crc(0xff) {
    DeclDate.day = 1;
    DeclDate.month = 1;
    DeclDate.year = 2010;
  }

protected:
  bool StartCommandMode();
  void StartNMEAMode(OperationEnvironment &env);
  void LoadPilotInfo(const Declaration &declaration);
  void WritePilotInfo();
  bool LoadTask(const Declaration &declaration);
  void WriteTask();
  void LoadContestClass(const Declaration &declaration);
  void WriteContestClass();
  void CRCWriteint32(int32_t i);
  void CRCWrite(const uint8_t *buff, unsigned size);
  void CRCWrite(uint8_t c);

  struct lxDevice_Pilot_t { //strings have extra byte for NULL
    uint8_t unknown1[3];
    char PilotName[19];
    char GliderType[12];
    char GliderID[8];
    char CompetitionID[4];
    uint8_t unknown2[73];
  } gcc_packed;

  struct lxDevice_Declaration_t { //strings have extra byte for NULL
    uint8_t unknown1[5];
    uint8_t dayinput;
    uint8_t monthinput;
    uint8_t yearinput;
    uint8_t dayuser;
    uint8_t monthuser;
    uint8_t yearuser;
    int16_t taskid;
    uint8_t numtps;
    uint8_t tptypes[12];
    int32_t Longitudes[12];
    int32_t Latitudes[12];
    char WaypointNames[12][9];
  } gcc_packed;


  struct lxDevice_ContestClass_t { //strings have extra byte for NULL
    char contest_class[9];
  } gcc_packed;

  lxDevice_Declaration_t lxDevice_Declaration;
  lxDevice_Pilot_t lxDevice_Pilot;
  lxDevice_ContestClass_t lxDevice_ContestClass;
  uint8_t crc;

  bool DeclareInner(const Declaration &declaration,
                    OperationEnvironment &env);

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO &info);
  virtual bool PutMacCready(fixed MacCready);
  virtual bool Declare(const Declaration &declaration,
                       OperationEnvironment &env);
};

#endif
