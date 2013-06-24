/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef VEGAVOICE_HPP
#define VEGAVOICE_HPP

#include "Math/fixed.hpp"
#include "Thread/Mutex.hpp"
#include "Compiler.h"

#include <tchar.h>

struct NMEAInfo;
struct DerivedInfo;
struct VoiceSettings;

// These messages are listed in order of priotity,
// first message to be active is spoken.
enum {
       // single play messages
       VV_GENERAL=0,
       VV_MACCREADY,
       VV_NEWWAYPOINT,
       VV_AIRSPACE,

       // repetitive play messages
       VV_CLIMBRATE,
       VV_INSECTOR,
       VV_WAYPOINTDISTANCE,
       VV_TERRAIN,

       // dummy, used to size array
       VV_MESSAGE_COUNT
};


class VegaVoiceMessage {
  bool active;
  // indicates whether this message needs to be spoken

  int repeatInterval;
  // repeat rate of message.

  bool singleplay;

  int alarmlevel;

  int id;
  // message id

  static int id_active;
  // message id of the message we want Vega to speak next

  // time the message was spoken as reported by Vega
  fixed lastTime;

  static TCHAR last_messageText[80];
  // buffer for message

  TCHAR messageText[80];

public:
  void Initialise(int the_id);

private:
  void MessageHeader();

  void SendMessage();

  bool TimeReady(fixed time);

  void DoSend(fixed time, TCHAR *text);

public:
  void SendNullMessage();

  // Called when the Vega Ack button is pressed
  void Acknowledge(fixed time);

  // called when Vega reports that the message was spoken
  void MessageSpoken(fixed time);


public:

  bool Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const VoiceSettings &settings);

};


class VegaVoice {
  static bool AirspaceNotifierInstalled;

  Mutex mutexVoice;

  VegaVoiceMessage message[VV_MESSAGE_COUNT];

public:
  VegaVoice();
  ~VegaVoice();

  void Update(const NMEAInfo &basic, const DerivedInfo &calculated,
              const VoiceSettings &settings);

  // called when notified by Altair that the message has been spoken
  void MessageSpoken(int id_this, fixed time);
};

#endif
