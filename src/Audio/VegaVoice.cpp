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

#include "Audio/VegaVoice.hpp"
#include "SettingsComputer.hpp"
#include "Units/Units.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"

#include <stdio.h>

enum {
  VWI_ZERO=20,
  VWI_ONE=24,
  VWI_TWO=27,
  VWI_THREE=28,
  VWI_FOUR=29,
  VWI_FIVE=30,
  VWI_SIX=31,
  VWI_SEVEN=32,
  VWI_EIGHT=33,
  VWI_NINE=34,
  //
  VWI_INFO=64,
  VWI_WARNING=66,
  VWI_AIRSPACE=62,
  VWI_CIRCLING=72,
  VWI_PLUS=41,

  VWI_ABOVE=55,
  VWI_BELOW=56,

  // Not yet implemented
  VWI_THOUSANDS=0,
  VWI_HUNDREDS=0,
};


void VegaVoiceMessage::TextToDigitsSmall(TCHAR *text, double number) {
  TCHAR tdigit[80];
  int ntens;
  int nones;
  int ndecimals;

  ntens = (int)(number/10);
  nones = (int)(number-ntens*10);
  ndecimals = (int)((number-ntens*10-nones)*10);

  if (ntens>0) {
    _stprintf(tdigit, _T(",%d"), LookupDigit(ntens));
    _tcscat(text,tdigit);
  }

  _stprintf(tdigit, _T(",%d"), LookupDigit(nones));
  _tcscat(text,tdigit);

  _stprintf(tdigit, _T(",%d"), LookupDigit(ndecimals));
  _tcscat(text,tdigit);

}


void VegaVoiceMessage::TextToDigitsLarge(TCHAR *text, double number) {
  TCHAR tdigit[80];
  int nhundreds;
  int ntens;
  int nones;

  number = lround(number);

  nhundreds = (int)(number/100);
  ntens = (int)((number-nhundreds*10)/10);
  nones = (int)(number-ntens*10-nhundreds*100);

  if (nhundreds>0) {
    _stprintf(tdigit, _T(",%d"), LookupDigit(nhundreds));
    _tcscat(text,tdigit);
  }
  if ((nhundreds>0)||(ntens>0)) {
    _stprintf(tdigit, _T(",%d"), LookupDigit(ntens));
    _tcscat(text,tdigit);
  }
  _stprintf(tdigit, _T(",%d"), LookupDigit(nones));
  _tcscat(text,tdigit);

}


void VegaVoiceMessage::TextToDigitsHuge(TCHAR *text, double number) {
  TCHAR tdigit[80];
  int nthousands;
  int nhundreds;

  number = lround(number);

  nthousands = (int)(number/1000);
  nhundreds = (int)((number-nthousands*10)/100);

  if (nthousands>0) {
    _stprintf(tdigit, _T(",%d,%d"), LookupDigit(nthousands), VWI_THOUSANDS);
    _tcscat(text,tdigit);
  }
  if (nhundreds>0) {
    _stprintf(tdigit, _T(",%d,%d"), LookupDigit(nhundreds), VWI_HUNDREDS);
    _tcscat(text,tdigit);
  }
}



int VegaVoiceMessage::LookupDigit(int number) {
  switch(number) {
  case 0:
    return VWI_ZERO;
  case 1:
    return VWI_ONE;
  case 2:
    return VWI_TWO;
  case 3:
    return VWI_THREE;
  case 4:
    return VWI_FOUR;
  case 5:
    return VWI_FIVE;
  case 6:
    return VWI_SIX;
  case 7:
    return VWI_SEVEN;
  case 8:
    return VWI_EIGHT;
  case 9:
    return VWI_NINE;
  };
  // shouldn't get here
  return VWI_ZERO;
}

void VegaVoiceMessage::Initialise(int the_id) {
  last_messageText[0]=0;
  id_active = -1;

  lastTime = 0.0;
  id = the_id;
  active = false;
  alarmlevel = 0;

  repeatInterval = 30;

  switch(id) {
  case VV_GENERAL:
    singleplay = true;
    break;
  case VV_MACCREADY:
    singleplay = true;
    break;
  case VV_NEWWAYPOINT:
    singleplay = true;
    break;
  case VV_AIRSPACE:
    singleplay = true;
    alarmlevel = 1;
    break;
  case VV_WAYPOINTDISTANCE:
    singleplay = false;
    repeatInterval = 120;
    break;
  case VV_TERRAIN:
    singleplay = false;
    repeatInterval = 120;
    break;
  case VV_INSECTOR:
    singleplay = false;
    repeatInterval = 60;
    break;
  case VV_CLIMBRATE:
    singleplay = false;
    repeatInterval = 60;
    break;
  default:
    singleplay = true;
    break;
  };

}

void VegaVoiceMessage::MessageHeader() {
  _stprintf(messageText, _T("PDVMS,%d,%d,%d,%d"),
            id, // message ID
            alarmlevel,
            repeatInterval * 1000,
            singleplay);
}

void VegaVoiceMessage::SendMessage() {



  /*
  if (_tcscmp(messageText, last_messageText)==0) {
    // no change, no need to send
    return;
  }

  _tcscpy(last_messageText, messageText);

  */



//  AllDevicesPutVoice(messageText);

}


void VegaVoiceMessage::SendNullMessage() {
  id_active = -1;
  _stprintf(messageText, _T("PDVMS,-1"));
  SendMessage();
}


// Called when the Vega Ack button is pressed
// TODO enhancement: prevent the message coming up again unless it
// becomes inactive for a while and then active again
void VegaVoiceMessage::Acknowledge(double time) {
  if (active) {
    active = false;
  }
  if (id_active == id) {
    lastTime = time;
    // Send cancel message
    SendNullMessage();
  }

}


// called when Vega reports that the message was spoken
void VegaVoiceMessage::MessageSpoken(double time) {
  lastTime = time;
}


bool VegaVoiceMessage::TimeReady(double time) {
  if (time-lastTime>repeatInterval) {
    return true;
  } else {
    return false;
  }
}


void
VegaVoiceMessage::DoSend(gcc_unused double time, TCHAR *text)
{
  /*

  // this ensures that if a repeated message is played,
  // and another message from XCSoar interrupts it,
  // and then it returns to this message type, it will
  // wait until the proper repeat time.
  if (id_active != id) {
    if (TimeReady(time)) {
      // need to send 'first' update
      id_active = id;
    } else {
      if (active) {
	      // message was already active when it may have been
	      // interrupted, so wait until time is ready for this message
	      return;
      } else {
	      // if message wasn't already active, it is now,
	      // so it should be played immediately
	      lastTime = time;
      }
    }
  }

  */


  id_active = id;
  active = true;

  MessageHeader();
  _tcscat(messageText,text);
  SendMessage();
}


bool
VegaVoiceMessage::Update(const NMEA_INFO &basic,
                         const DerivedInfo &calculated,
			 const SETTINGS_COMPUTER &settings)
{
  const fixed Time = basic.clock;
  TCHAR text[80];

  switch(id) {
  case VV_GENERAL:
    // TODO feature: Allow this to be triggered to give generic alert
    // "INFO"
    break;
  case VV_CLIMBRATE:
    if (!settings.EnableVoiceClimbRate) return false;

    if (calculated.circling && positive(calculated.average)) {
      // Gives the average climb rate in user units every X seconds
      // while in circling mode
      // e.g. if average = 3.4 (user units)
      // Now: "CIRCLING THREE FOUR"
      // Later: "AVERAGE THREE POINT FOUR"
      _stprintf(text, _T(",%d"), VWI_CIRCLING);
      TextToDigitsSmall(text, Units::ToUserVSpeed(calculated.average));
      DoSend(Time, text);
      return true;
    }
    break;
  case VV_TERRAIN:
    if (!settings.EnableVoiceTerrain) return false;
    // TODO feature: final glide with terrain warning
    // CAUTION TERRAIN
    break;
  case VV_WAYPOINTDISTANCE:
#ifdef OLD_TASK
    if (!calculated.circling && task.Valid()) {

      // Gives the distance to the active waypoint every X seconds,
      // optionally limited when at last 20 km to go?
      // calculated.LegDistanceToGo
      // e.g.: if distance is 13 (user units)
      // Now: "PLUS ONE THREE"
      // Later: "WAYPOINT DISTANCE THREE FOUR"
      //
      // height above/below final glide when
      // in final glide mode, e.g.
      // "WAYPOINT BELOW TWO HUNDRED"

      if (Units::ToUserUnit(calculated.WaypointDistance,
                            Units::DistanceUnit) < 20.0) {

	      if (!settings.EnableVoiceWaypointDistance) return false;

              _stprintf(text, _T(",%d"), VWI_PLUS);
	      TextToDigitsLarge(text, Units::ToUserUnit(calculated.WaypointDistance,
                                                  Units::DistanceUnit));
              DoSend(Time, text);
	      return true;
      } else {

	      if (calculated.FinalGlide) {

	        if (!settings.EnableVoiceTaskAltitudeDifference) return false;

	        // TODO feature: BELOW FOUR HUNDRED
	        double tad = Units::ToUserUnit(calculated.TaskAltitudeDifference,
                                         Units::AltitudeUnit);
	        if (fabs(tad)>100) {
	          if (tad>0) {
                    _stprintf(text, _T(",%d"), VWI_ABOVE);
	          } else {
                    _stprintf(text, _T(",%d"), VWI_BELOW);
	          }
	          TextToDigitsHuge(text, fabs(tad));
                  DoSend(Time, text);
	          return true;
	        }
	      }

      }
    }
#endif
    break;
  case VV_MACCREADY:
    if (!settings.EnableVoiceMacCready) return false;
    // TODO feature: report when not in auto maccready mode, if
    // vario has changed in last 3 seconds but hasn't changed
    // for more than one second
    //
    // Now: ---
    // Later: "MACCREADY THREE DECIMAL FOUR"
    break;
  case VV_NEWWAYPOINT:
    if (!settings.EnableVoiceNewWaypoint) return false;
#ifdef OLD_TASK
    static unsigned LastWaypoint = 1000;
    if (task.getActiveIndex() != LastWaypoint) {
      LastWaypoint = task.getActiveIndex();
      // Reports that a new waypoint is active
      // e.g.:
      // Now: "INFO"
      // Later: "NEW WAYPOINT"
      _stprintf(text, _T(",%d"), VWI_INFO);
      DoSend(Time, text);
      return true;
    }
#endif
    break;
  case VV_INSECTOR:
    if (!settings.EnableVoiceInSector) return false;
#ifdef OLD_TASK
    if (calculated.IsInSector) {
      // Reports when the aircraft is in an AAT/task sector
      // e.g.:
      // Now: INFO
      // Later: "INSIDE SECTOR"
      _stprintf(text, _T(",%d"), VWI_INFO);
      DoSend(Time, text);
      return true;
    }
#endif
    break;
  case VV_AIRSPACE:
    if (!settings.EnableVoiceAirspace) return false;
#ifdef OLD_TASK
    if (calculated.IsInAirspace) {
      // Reports when the aircraft is inside airspace
      // e.g.:
      // Now: "WARNING AIRSPACE"
      //
      // Later give distance/height/direction?
      // Later: "WARNING AIRSPACE ABOVE"
      _stprintf(text, _T(",%d,%d,0"), VWI_WARNING, VWI_AIRSPACE);
      DoSend(Time, text);
      return true;
    }
#endif
    break;
  default:
    break;
  };
  return false;
}


int VegaVoiceMessage::id_active = -1;
TCHAR VegaVoiceMessage::last_messageText[80];

#ifdef OLD_TASK

static void AirspaceWarningNotify(AirspaceWarningNotifyAction_t Action, AirspaceInfo_c *AirSpace){
  (void)AirSpace;
  static bool PlaySimpleWarning = false;

  if (!globalRunningEvent.test())
    return;

  switch (Action){
  case asaNull:
  case asaProcessBegin:
    break;

    case asaItemAdded:

    case asaWarnLevelIncreased:

      PlaySimpleWarning = true;

    case asaItemRemoved:

    case asaClearAll:

    case asaItemChanged:

    break;



    case asaProcessEnd:

      if (PlaySimpleWarning){

        PlaySimpleWarning = false;

        InputEvents::eventBeep(TEXT("1"));

      }
    break;



  }



}

#endif

VegaVoice::VegaVoice() {
  for (int i=0; i<VV_MESSAGE_COUNT; i++) {
    message[i].Initialise(i);
  }
}

bool VegaVoice::AirspaceNotifierInstalled = false;

VegaVoice::~VegaVoice() {
#ifdef OLD_TASK
  if (AirspaceNotifierInstalled) {
    AirspaceNotifierInstalled = false;
    AirspaceWarnListRemoveNotifier(AirspaceWarningNotify);
  }
#endif
}

void
VegaVoice::Update(const NMEA_INFO &basic, const DerivedInfo &calculated,
		  const SETTINGS_COMPUTER &settings)
{

  if (!AirspaceNotifierInstalled){
#ifdef OLD_TASK
    AirspaceNotifierInstalled = true;

    // note this isn't removed yet on destruction, so it is a very small
    // memory leak
    AirspaceWarnListAddNotifier(AirspaceWarningNotify);
#endif
  }

  ScopeLock protect(mutexVoice);

  // update values in each message to determine whether
  // the message should be active
  for (int i=0; i<VV_MESSAGE_COUNT; i++)
    if (message[i].Update(basic, calculated, settings))
      return;

  // no message is active now
  // need to send null message (cancel all)
  // message[0].SendNullMessage();
}


// called when notified by Altair that the message has been spoken
void VegaVoice::MessageSpoken(int id_this, double time) {
  ScopeLock protect(mutexVoice);

  if ((id_this>=0)&&(id_this<VV_MESSAGE_COUNT)) {
    message[id_this].MessageSpoken(time);
  }
}


