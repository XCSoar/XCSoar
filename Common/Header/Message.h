#ifndef MESSAGE_H
#define MESSAGE_H

#include "StdAfx.h"
#include "XCSoar.h"

#define MAXMESSAGES 20

enum {
  MSG_UNKNOWN=0,
  MSG_AIRSPACE,
  MSG_USERINTERFACE,
  MSG_GLIDECOMPUTER,
  MSG_COMMS
};


struct singleMessage {
  TCHAR text[1000];
  int type;
  DWORD tstart; // time message was created
  DWORD texpiry; // time message will expire
  DWORD tshow; // time message is visible for
};


class Message {
 public:
  static void Initialize(RECT rc);
  static void Destroy();
  static bool Render(); // returns true if messages have changed

  static void AddMessage(DWORD tshow, int type, TCHAR *Text);

  // repeats last non-visible message of specified type (or any message
  // type=0)
  static void Repeat(int type);

  // clears all visible messages (of specified type or if type=0, all)
  static bool Acknowledge(int type);

  static void Lock();
  static void Unlock();

  static void CheckTouch(HWND wmControl);

  static void BlockRender(bool doblock);

 private:
  static struct singleMessage messages[MAXMESSAGES];
  static RECT rcmsg; // maximum message size
  static HWND hWndMessageWindow;
  static TCHAR msgText[2000];
  static HDC hdc;
  static void Resize();
  static int GetEmptySlot();
  static bool hidden;
  static int nvisible;
  static int block_ref;

};


#endif
