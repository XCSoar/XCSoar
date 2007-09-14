#ifndef VEGAVOICE_H
#define VEGAVOICE_H

#include "stdafx.h"
#include <windows.h>
#include "Parser.h"
#include "Calculations.h"
#include "externs.h"

extern bool EnableVoiceClimbRate;
extern bool EnableVoiceTerrain;
extern bool EnableVoiceWaypointDistance;
extern bool EnableVoiceTaskAltitudeDifference;
extern bool EnableVoiceMacCready;
extern bool EnableVoiceNewWaypoint;
extern bool EnableVoiceInSector;
extern bool EnableVoiceAirspace;

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
public:
  void Initialise(int the_id);

private:

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
  double lastTime;

  static TCHAR last_messageText[80];
  // buffer for message

  TCHAR messageText[80];

  void MessageHeader();

  void SendMessage();

  bool TimeReady(double time);

  void DoSend(double time, TCHAR *text);

  void TextToDigitsSmall(TCHAR *text, double number);

  void TextToDigitsLarge(TCHAR *text, double number);

  void TextToDigitsHuge(TCHAR *text, double number);

  int LookupDigit(int number);

public:
  void SendNullMessage();

  // Called when the Vega Ack button is pressed
  void Acknowledge(double time);

  // called when Vega reports that the message was spoken
  void MessageSpoken(double time);


public:

  bool Update(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

};


class VegaVoice {
public:
  VegaVoice();
  ~VegaVoice();

  VegaVoiceMessage message[VV_MESSAGE_COUNT];

  void Update(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

  // called when notified by Altair that the message has been spoken
  void MessageSpoken(int id_this, double time);

private:
  CRITICAL_SECTION  CritSec_Voice;
  void Lock();
  void UnLock();
 private:
  static bool AirspaceNotifierInstalled;
};

#endif
