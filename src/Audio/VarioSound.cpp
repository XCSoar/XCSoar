// VarioSound.cpp : Defines the entry point for the DLL application.
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


#include "VarioSound.h"
#include "Math/Constants.h"

#include <math.h>

BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
		       )
{
  switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
    }
  return TRUE;
}


#include <stdlib.h>

#ifdef DEBUGAUDIO
#endif

double randomgaussian() {
#ifdef SIMULATENOISE
  double k1;
  double k2 = rand()*1.0/RAND_MAX;
  double rval;
  do {
    k1 = rand()*1.0/RAND_MAX;
  } while (k1==0);

  rval = sqrt(-2*log(k1))*cos(2.0*M_PI*k2);
  return rval;
#else
  return 0.0;
#endif
}

#include "WaveThread.h"

CWaveOutThread variosound_waveOut;

#define CHANNELS 2
#define BSIZE 1
#define BCOUNT INTERNAL_WAVEOUT_BUFFER_COUNT
#define FREQZ 80*5
#define FREQ 8000

/**
 * Audio digital filter parameters.
 * 
 */
struct AudioFilter {
  bool stfmode;
  short vcur;
  short vsmooth;
  short vstfsmooth;
  short vsmoothlast;
  short vstfsmoothlast;
};

struct AudioFilter audiofilter = {false, 100, 0, 0, 100, 0};

unsigned char variosound_buffer[BSIZE*FREQZ*CHANNELS];


unsigned char audio_volume = 8;

void VarioSound_synthesiseSound();

short variosound_vscale_in = 0;
short variosound_vav_in = 0;
short variosound_valt = 0;

short variosound_vscale=0;
short variosound_vscale_last=0;
short variosound_vcur = 0;
short variosound_vav=0;
short variosound_vscale_timer=0;
BOOL variosound_sound = TRUE;

DWORD fpsTimeSound = 0; // time stamp of sound command
short fpsTimeDelta = 0; // time interval between sound commands



CRITICAL_SECTION  CritSec_VarioSound;
CRITICAL_SECTION  CritSec_VarioSoundV;

unsigned short audio_beepfrequency = 17;
unsigned short audio_soundfrequency = 3276;
unsigned short audio_altsoundfrequency = 3276;
unsigned char audio_soundtype = 0; // start in silence
unsigned char audio_deadband_hi = 100;
unsigned char audio_deadband_low = 100;

// Corrections for phase, must be global.
unsigned char audio_phase = 0;
unsigned short audio_phase_i = 0;
unsigned char audio_phase_new = 0;
unsigned short audio_phase_i_new = 0;

unsigned char audio_sintable(unsigned char c);
unsigned char audio_sintable1(unsigned char c);
unsigned char audio_sintable2(unsigned char c);

short quantisesound(short vv) {
  return max(0,min(200,vv+100));
}

void audio_soundparameters(void) {
  // save last phase, so seamless stitching of sin waves
  // if this is not done, there will be crackles and pops when
  // the sound parameters change due to phase offsets

  audio_phase = audio_phase_new;
  audio_phase_i = audio_phase_i_new;
}

unsigned char audio_adjust_volume(unsigned char sval) {
  short oval = ((((short)sval-0x80)*(short)audio_volume)>>3)+0x80;
  return (unsigned char)oval;
}


// look up sin wave
//
unsigned char audio_sound_loud(unsigned short i, bool beep) {
  unsigned char phase;

  if (beep) {
    phase = (((i-audio_phase_i)*audio_soundfrequency) >> 8)+ audio_phase;
  } else {
    if (audio_soundfrequency != audio_altsoundfrequency) {
      phase = (((i-audio_phase_i)*audio_altsoundfrequency) >> 8)+ audio_phase;
    } else {
      return 0x80; // return silence
    }
  }

  // save last phase data...
  // (only need to do this just before switching freqs)
  audio_phase_new = phase;
  audio_phase_i_new = i;

  return audio_adjust_volume(audio_sintable1(phase));
}


// look up beep phase location
//
// this routine only has two sounds implemented:
//   0: silence
//   1: positive lift, short beep (60/256) followed by (196/256) quiet
//   2: negative lift, long beep (200/256) followed by (56/256) quiet
//   3: solid sound
// other types may be added later to implement hurry-up sounds etc.


struct AudioSoundStatic_t {
  unsigned Index:16;
  unsigned Last:1;
  unsigned WaitZerroCross:1;
  unsigned Advance:1;
};

static AudioSoundStatic_t BeepData = {0,0,0,0};


unsigned char audio_sound_beep(unsigned short i) {
  unsigned char phase;
  phase = (i*audio_beepfrequency) >> 8;

  BeepData.Advance = false;

  switch (audio_soundtype) {
  case 0: // silence
    BeepData.Advance=true;
    return 0;
  case 1: // positive sound, short beep
    if (phase<64) {
      BeepData.Advance=true;
      return 1;
    }
    else
      return 0;
  case 2: // negative sound, long beep
    if (phase<200) {
      BeepData.Advance=true;
      return 1;
    } else
      return 0;
  case 3: // solid sound
    BeepData.Advance=true;
    return 1;
  case 4: // two short beeps
    if (phase<24) {
      BeepData.Advance = true;
      return 1;
    }
    if ((phase>48)&&(phase<76)) {
      return 1;
    }
    return 0;
  case 5: // equal beep
    if (phase<128) {
      BeepData.Advance=true;
      return 1;
    } else
      return 0;
  }
  // unknown
  BeepData.Advance = true;
  return 0;
}


// Retrieve the next sound byte
//
unsigned char audio_get_sound_byte(void) {

  return 0x80;

  unsigned char beepthis;
  bool makebeepsound;

  // increment beep phase
  BeepData.Index++;

  // check if this sound type makes a beep
  beepthis = audio_sound_beep(BeepData.Index);

  // check to see if we can reset the beep phase data
  if (beepthis && (BeepData.Last==0) && (BeepData.Advance)) {
    // rolled over new beep and the sound mode allows advance,
    // so restart phase counter
    BeepData.Index=0;
    audio_phase = 0;
    audio_phase_i = 0;
  }
  BeepData.Last = beepthis;

  // continue to make a beep if waiting for zero crossing or it's a
  // real beep anyway
  makebeepsound = beepthis || BeepData.WaitZerroCross;

  // find the sound sample
  static unsigned char lastsample = 0;
  unsigned char sample = audio_sound_loud(BeepData.Index, makebeepsound);

  if (makebeepsound) {
    // wait for zero crossing before finishing beep
    BeepData.WaitZerroCross = beepthis  // still need a real beep
      || ((sample>0x80)&&(lastsample>0x80))
      || ((sample<0x80)&&(lastsample<0x80)); // better zero crossing detection
    if (!beepthis && !BeepData.WaitZerroCross) {
      // found zero crossing and need no beep now, so silence it.
      sample = 0x80;
    }
  }
  lastsample = sample;

  return(sample);
}

// sin function indexed by unsigned char.  This is a fast implementation.
//  (auto-generated from a perl script)
// This function could be modified to incorporate different wave forms
// for example to make the sound more harsh.
//

unsigned char audio_sintable2(unsigned char c) {
 switch (c) {
   case 0: return 128;
   case 1: return 128;
   case 2: return 128;
   case 3: return 128;
   case 4: return 128;
   case 5: return 128;
   case 6: return 129;
   case 7: return 129;
   case 8: return 130;
   case 9: return 130;
   case 10: return 131;
   case 11: return 132;
   case 12: return 133;
   case 13: return 134;
   case 14: return 136;
   case 15: return 137;
   case 16: return 139;
   case 17: return 141;
   case 18: return 143;
   case 19: return 145;
   case 20: return 147;
   case 21: return 149;
   case 22: return 151;
   case 23: return 154;
   case 24: return 156;
   case 25: return 159;
   case 26: return 162;
   case 27: return 165;
   case 28: return 168;
   case 29: return 171;
   case 30: return 174;
   case 31: return 177;
   case 32: return 180;
   case 33: return 184;
   case 34: return 187;
   case 35: return 190;
   case 36: return 194;
   case 37: return 197;
   case 38: return 200;
   case 39: return 204;
   case 40: return 207;
   case 41: return 210;
   case 42: return 213;
   case 43: return 216;
   case 44: return 220;
   case 45: return 223;
   case 46: return 225;
   case 47: return 228;
   case 48: return 231;
   case 49: return 233;
   case 50: return 236;
   case 51: return 238;
   case 52: return 240;
   case 53: return 242;
   case 54: return 244;
   case 55: return 246;
   case 56: return 248;
   case 57: return 249;
   case 58: return 250;
   case 59: return 251;
   case 60: return 252;
   case 61: return 253;
   case 62: return 253;
   case 63: return 253;
   case 64: return 253;
   case 65: return 253;
   case 66: return 253;
   case 67: return 253;
   case 68: return 252;
   case 69: return 251;
   case 70: return 250;
   case 71: return 249;
   case 72: return 248;
   case 73: return 246;
   case 74: return 244;
   case 75: return 242;
   case 76: return 240;
   case 77: return 238;
   case 78: return 236;
   case 79: return 233;
   case 80: return 231;
   case 81: return 228;
   case 82: return 225;
   case 83: return 223;
   case 84: return 220;
   case 85: return 216;
   case 86: return 213;
   case 87: return 210;
   case 88: return 207;
   case 89: return 204;
   case 90: return 200;
   case 91: return 197;
   case 92: return 194;
   case 93: return 190;
   case 94: return 187;
   case 95: return 184;
   case 96: return 180;
   case 97: return 177;
   case 98: return 174;
   case 99: return 171;
   case 100: return 168;
   case 101: return 165;
   case 102: return 162;
   case 103: return 159;
   case 104: return 156;
   case 105: return 154;
   case 106: return 151;
   case 107: return 149;
   case 108: return 147;
   case 109: return 145;
   case 110: return 143;
   case 111: return 141;
   case 112: return 139;
   case 113: return 137;
   case 114: return 136;
   case 115: return 134;
   case 116: return 133;
   case 117: return 132;
   case 118: return 131;
   case 119: return 130;
   case 120: return 130;
   case 121: return 129;
   case 122: return 129;
   case 123: return 128;
   case 124: return 128;
   case 125: return 128;
   case 126: return 128;
   case 127: return 128;
   case 128: return 128;
   case 129: return 127;
   case 130: return 127;
   case 131: return 127;
   case 132: return 127;
   case 133: return 127;
   case 134: return 126;
   case 135: return 126;
   case 136: return 125;
   case 137: return 125;
   case 138: return 124;
   case 139: return 123;
   case 140: return 122;
   case 141: return 121;
   case 142: return 119;
   case 143: return 118;
   case 144: return 116;
   case 145: return 114;
   case 146: return 112;
   case 147: return 110;
   case 148: return 108;
   case 149: return 106;
   case 150: return 104;
   case 151: return 101;
   case 152: return 99;
   case 153: return 96;
   case 154: return 93;
   case 155: return 90;
   case 156: return 87;
   case 157: return 84;
   case 158: return 81;
   case 159: return 78;
   case 160: return 75;
   case 161: return 71;
   case 162: return 68;
   case 163: return 65;
   case 164: return 61;
   case 165: return 58;
   case 166: return 55;
   case 167: return 51;
   case 168: return 48;
   case 169: return 45;
   case 170: return 42;
   case 171: return 39;
   case 172: return 35;
   case 173: return 32;
   case 174: return 30;
   case 175: return 27;
   case 176: return 24;
   case 177: return 22;
   case 178: return 19;
   case 179: return 17;
   case 180: return 15;
   case 181: return 13;
   case 182: return 11;
   case 183: return 9;
   case 184: return 7;
   case 185: return 6;
   case 186: return 5;
   case 187: return 4;
   case 188: return 3;
   case 189: return 2;
   case 190: return 2;
   case 191: return 2;
   case 192: return 2;
   case 193: return 2;
   case 194: return 2;
   case 195: return 2;
   case 196: return 3;
   case 197: return 4;
   case 198: return 5;
   case 199: return 6;
   case 200: return 7;
   case 201: return 9;
   case 202: return 11;
   case 203: return 13;
   case 204: return 15;
   case 205: return 17;
   case 206: return 19;
   case 207: return 22;
   case 208: return 24;
   case 209: return 27;
   case 210: return 30;
   case 211: return 32;
   case 212: return 35;
   case 213: return 39;
   case 214: return 42;
   case 215: return 45;
   case 216: return 48;
   case 217: return 51;
   case 218: return 55;
   case 219: return 58;
   case 220: return 61;
   case 221: return 65;
   case 222: return 68;
   case 223: return 71;
   case 224: return 75;
   case 225: return 78;
   case 226: return 81;
   case 227: return 84;
   case 228: return 87;
   case 229: return 90;
   case 230: return 93;
   case 231: return 96;
   case 232: return 99;
   case 233: return 101;
   case 234: return 104;
   case 235: return 106;
   case 236: return 108;
   case 237: return 110;
   case 238: return 112;
   case 239: return 114;
   case 240: return 116;
   case 241: return 118;
   case 242: return 119;
   case 243: return 121;
   case 244: return 122;
   case 245: return 123;
   case 246: return 124;
   case 247: return 125;
   case 248: return 125;
   case 249: return 126;
   case 250: return 126;
   case 251: return 127;
   case 252: return 127;
   case 253: return 127;
   case 254: return 127;
   case 255: return 127;
 };
 return 128; // never get here
}


unsigned char audio_sintable1(unsigned char c) {
 switch (c) {
   case 0: return 128;
   case 1: return 147;
   case 2: return 155;
   case 3: return 162;
   case 4: return 167;
   case 5: return 172;
   case 6: return 176;
   case 7: return 180;
   case 8: return 183;
   case 9: return 186;
   case 10: return 190;
   case 11: return 193;
   case 12: return 195;
   case 13: return 198;
   case 14: return 201;
   case 15: return 203;
   case 16: return 205;
   case 17: return 208;
   case 18: return 210;
   case 19: return 212;
   case 20: return 214;
   case 21: return 216;
   case 22: return 218;
   case 23: return 220;
   case 24: return 221;
   case 25: return 223;
   case 26: return 225;
   case 27: return 226;
   case 28: return 228;
   case 29: return 229;
   case 30: return 231;
   case 31: return 232;
   case 32: return 233;
   case 33: return 235;
   case 34: return 236;
   case 35: return 237;
   case 36: return 238;
   case 37: return 239;
   case 38: return 240;
   case 39: return 241;
   case 40: return 242;
   case 41: return 243;
   case 42: return 244;
   case 43: return 245;
   case 44: return 246;
   case 45: return 247;
   case 46: return 247;
   case 47: return 248;
   case 48: return 249;
   case 49: return 249;
   case 50: return 250;
   case 51: return 250;
   case 52: return 251;
   case 53: return 251;
   case 54: return 252;
   case 55: return 252;
   case 56: return 252;
   case 57: return 253;
   case 58: return 253;
   case 59: return 253;
   case 60: return 253;
   case 61: return 253;
   case 62: return 253;
   case 63: return 253;
   case 64: return 253;
   case 65: return 253;
   case 66: return 253;
   case 67: return 253;
   case 68: return 253;
   case 69: return 253;
   case 70: return 253;
   case 71: return 253;
   case 72: return 252;
   case 73: return 252;
   case 74: return 252;
   case 75: return 251;
   case 76: return 251;
   case 77: return 250;
   case 78: return 250;
   case 79: return 249;
   case 80: return 249;
   case 81: return 248;
   case 82: return 247;
   case 83: return 247;
   case 84: return 246;
   case 85: return 245;
   case 86: return 244;
   case 87: return 243;
   case 88: return 242;
   case 89: return 241;
   case 90: return 240;
   case 91: return 239;
   case 92: return 238;
   case 93: return 237;
   case 94: return 236;
   case 95: return 235;
   case 96: return 233;
   case 97: return 232;
   case 98: return 231;
   case 99: return 229;
   case 100: return 228;
   case 101: return 226;
   case 102: return 225;
   case 103: return 223;
   case 104: return 221;
   case 105: return 220;
   case 106: return 218;
   case 107: return 216;
   case 108: return 214;
   case 109: return 212;
   case 110: return 210;
   case 111: return 208;
   case 112: return 205;
   case 113: return 203;
   case 114: return 201;
   case 115: return 198;
   case 116: return 195;
   case 117: return 193;
   case 118: return 190;
   case 119: return 186;
   case 120: return 183;
   case 121: return 180;
   case 122: return 176;
   case 123: return 172;
   case 124: return 167;
   case 125: return 162;
   case 126: return 155;
   case 127: return 147;
   case 128: return 128;
   case 129: return 108;
   case 130: return 100;
   case 131: return 93;
   case 132: return 88;
   case 133: return 83;
   case 134: return 79;
   case 135: return 75;
   case 136: return 72;
   case 137: return 69;
   case 138: return 65;
   case 139: return 62;
   case 140: return 60;
   case 141: return 57;
   case 142: return 54;
   case 143: return 52;
   case 144: return 50;
   case 145: return 47;
   case 146: return 45;
   case 147: return 43;
   case 148: return 41;
   case 149: return 39;
   case 150: return 37;
   case 151: return 35;
   case 152: return 34;
   case 153: return 32;
   case 154: return 30;
   case 155: return 29;
   case 156: return 27;
   case 157: return 26;
   case 158: return 24;
   case 159: return 23;
   case 160: return 22;
   case 161: return 20;
   case 162: return 19;
   case 163: return 18;
   case 164: return 17;
   case 165: return 16;
   case 166: return 15;
   case 167: return 14;
   case 168: return 13;
   case 169: return 12;
   case 170: return 11;
   case 171: return 10;
   case 172: return 9;
   case 173: return 8;
   case 174: return 8;
   case 175: return 7;
   case 176: return 6;
   case 177: return 6;
   case 178: return 5;
   case 179: return 5;
   case 180: return 4;
   case 181: return 4;
   case 182: return 3;
   case 183: return 3;
   case 184: return 3;
   case 185: return 2;
   case 186: return 2;
   case 187: return 2;
   case 188: return 2;
   case 189: return 2;
   case 190: return 2;
   case 191: return 2;
   case 192: return 2;
   case 193: return 2;
   case 194: return 2;
   case 195: return 2;
   case 196: return 2;
   case 197: return 2;
   case 198: return 2;
   case 199: return 2;
   case 200: return 3;
   case 201: return 3;
   case 202: return 3;
   case 203: return 4;
   case 204: return 4;
   case 205: return 5;
   case 206: return 5;
   case 207: return 6;
   case 208: return 6;
   case 209: return 7;
   case 210: return 8;
   case 211: return 8;
   case 212: return 9;
   case 213: return 10;
   case 214: return 11;
   case 215: return 12;
   case 216: return 13;
   case 217: return 14;
   case 218: return 15;
   case 219: return 16;
   case 220: return 17;
   case 221: return 18;
   case 222: return 19;
   case 223: return 20;
   case 224: return 22;
   case 225: return 23;
   case 226: return 24;
   case 227: return 26;
   case 228: return 27;
   case 229: return 29;
   case 230: return 30;
   case 231: return 32;
   case 232: return 34;
   case 233: return 35;
   case 234: return 37;
   case 235: return 39;
   case 236: return 41;
   case 237: return 43;
   case 238: return 45;
   case 239: return 47;
   case 240: return 50;
   case 241: return 52;
   case 242: return 54;
   case 243: return 57;
   case 244: return 60;
   case 245: return 62;
   case 246: return 65;
   case 247: return 69;
   case 248: return 72;
   case 249: return 75;
   case 250: return 79;
   case 251: return 83;
   case 252: return 88;
   case 253: return 93;
   case 254: return 100;
   case 255: return 108;
 };
 return 128; // never get here
}


unsigned char audio_sintable(unsigned char c) {
 switch (c) {
   case 0: return 128;
   case 1: return 131;
   case 2: return 134;
   case 3: return 137;
   case 4: return 140;
   case 5: return 143;
   case 6: return 146;
   case 7: return 149;
   case 8: return 152;
   case 9: return 155;
   case 10: return 158;
   case 11: return 161;
   case 12: return 164;
   case 13: return 167;
   case 14: return 170;
   case 15: return 173;
   case 16: return 176;
   case 17: return 179;
   case 18: return 181;
   case 19: return 184;
   case 20: return 187;
   case 21: return 190;
   case 22: return 192;
   case 23: return 195;
   case 24: return 198;
   case 25: return 200;
   case 26: return 203;
   case 27: return 205;
   case 28: return 207;
   case 29: return 210;
   case 30: return 212;
   case 31: return 214;
   case 32: return 217;
   case 33: return 219;
   case 34: return 221;
   case 35: return 223;
   case 36: return 225;
   case 37: return 227;
   case 38: return 229;
   case 39: return 231;
   case 40: return 232;
   case 41: return 234;
   case 42: return 236;
   case 43: return 237;
   case 44: return 239;
   case 45: return 240;
   case 46: return 241;
   case 47: return 243;
   case 48: return 244;
   case 49: return 245;
   case 50: return 246;
   case 51: return 247;
   case 52: return 248;
   case 53: return 249;
   case 54: return 250;
   case 55: return 250;
   case 56: return 251;
   case 57: return 252;
   case 58: return 252;
   case 59: return 253;
   case 60: return 253;
   case 61: return 253;
   case 62: return 253;
   case 63: return 253;
   case 64: return 253;
   case 65: return 253;
   case 66: return 253;
   case 67: return 253;
   case 68: return 253;
   case 69: return 253;
   case 70: return 252;
   case 71: return 252;
   case 72: return 251;
   case 73: return 250;
   case 74: return 250;
   case 75: return 249;
   case 76: return 248;
   case 77: return 247;
   case 78: return 246;
   case 79: return 245;
   case 80: return 244;
   case 81: return 243;
   case 82: return 241;
   case 83: return 240;
   case 84: return 239;
   case 85: return 237;
   case 86: return 236;
   case 87: return 234;
   case 88: return 232;
   case 89: return 231;
   case 90: return 229;
   case 91: return 227;
   case 92: return 225;
   case 93: return 223;
   case 94: return 221;
   case 95: return 219;
   case 96: return 217;
   case 97: return 214;
   case 98: return 212;
   case 99: return 210;
   case 100: return 207;
   case 101: return 205;
   case 102: return 203;
   case 103: return 200;
   case 104: return 198;
   case 105: return 195;
   case 106: return 192;
   case 107: return 190;
   case 108: return 187;
   case 109: return 184;
   case 110: return 181;
   case 111: return 179;
   case 112: return 176;
   case 113: return 173;
   case 114: return 170;
   case 115: return 167;
   case 116: return 164;
   case 117: return 161;
   case 118: return 158;
   case 119: return 155;
   case 120: return 152;
   case 121: return 149;
   case 122: return 146;
   case 123: return 143;
   case 124: return 140;
   case 125: return 137;
   case 126: return 134;
   case 127: return 131;
   case 128: return 128;
   case 129: return 124;
   case 130: return 121;
   case 131: return 118;
   case 132: return 115;
   case 133: return 112;
   case 134: return 109;
   case 135: return 106;
   case 136: return 103;
   case 137: return 100;
   case 138: return 97;
   case 139: return 94;
   case 140: return 91;
   case 141: return 88;
   case 142: return 85;
   case 143: return 82;
   case 144: return 79;
   case 145: return 76;
   case 146: return 74;
   case 147: return 71;
   case 148: return 68;
   case 149: return 65;
   case 150: return 63;
   case 151: return 60;
   case 152: return 57;
   case 153: return 55;
   case 154: return 52;
   case 155: return 50;
   case 156: return 48;
   case 157: return 45;
   case 158: return 43;
   case 159: return 41;
   case 160: return 38;
   case 161: return 36;
   case 162: return 34;
   case 163: return 32;
   case 164: return 30;
   case 165: return 28;
   case 166: return 26;
   case 167: return 24;
   case 168: return 23;
   case 169: return 21;
   case 170: return 19;
   case 171: return 18;
   case 172: return 16;
   case 173: return 15;
   case 174: return 14;
   case 175: return 12;
   case 176: return 11;
   case 177: return 10;
   case 178: return 9;
   case 179: return 8;
   case 180: return 7;
   case 181: return 6;
   case 182: return 5;
   case 183: return 5;
   case 184: return 4;
   case 185: return 3;
   case 186: return 3;
   case 187: return 2;
   case 188: return 2;
   case 189: return 2;
   case 190: return 2;
   case 191: return 2;
   case 192: return 2;
   case 193: return 2;
   case 194: return 2;
   case 195: return 2;
   case 196: return 2;
   case 197: return 2;
   case 198: return 3;
   case 199: return 3;
   case 200: return 4;
   case 201: return 5;
   case 202: return 5;
   case 203: return 6;
   case 204: return 7;
   case 205: return 8;
   case 206: return 9;
   case 207: return 10;
   case 208: return 11;
   case 209: return 12;
   case 210: return 14;
   case 211: return 15;
   case 212: return 16;
   case 213: return 18;
   case 214: return 19;
   case 215: return 21;
   case 216: return 23;
   case 217: return 24;
   case 218: return 26;
   case 219: return 28;
   case 220: return 30;
   case 221: return 32;
   case 222: return 34;
   case 223: return 36;
   case 224: return 38;
   case 225: return 41;
   case 226: return 43;
   case 227: return 45;
   case 228: return 48;
   case 229: return 50;
   case 230: return 52;
   case 231: return 55;
   case 232: return 57;
   case 233: return 60;
   case 234: return 63;
   case 235: return 65;
   case 236: return 68;
   case 237: return 71;
   case 238: return 74;
   case 239: return 76;
   case 240: return 79;
   case 241: return 82;
   case 242: return 85;
   case 243: return 88;
   case 244: return 91;
   case 245: return 94;
   case 246: return 97;
   case 247: return 100;
   case 248: return 103;
   case 249: return 106;
   case 250: return 109;
   case 251: return 112;
   case 252: return 115;
   case 253: return 118;
   case 254: return 121;
   case 255: return 124;
 };
 return 128; // never get here
}

unsigned short audio_frequencytable(unsigned char c) {
 switch (c) {
   case 0: return 1638;
   case 1: return 1649;
   case 2: return 1661;
   case 3: return 1672;
   case 4: return 1684;
   case 5: return 1696;
   case 6: return 1707;
   case 7: return 1719;
   case 8: return 1731;
   case 9: return 1743;
   case 10: return 1755;
   case 11: return 1768;
   case 12: return 1780;
   case 13: return 1792;
   case 14: return 1805;
   case 15: return 1817;
   case 16: return 1830;
   case 17: return 1843;
   case 18: return 1856;
   case 19: return 1869;
   case 20: return 1882;
   case 21: return 1895;
   case 22: return 1908;
   case 23: return 1921;
   case 24: return 1934;
   case 25: return 1948;
   case 26: return 1961;
   case 27: return 1975;
   case 28: return 1989;
   case 29: return 2003;
   case 30: return 2017;
   case 31: return 2031;
   case 32: return 2045;
   case 33: return 2059;
   case 34: return 2073;
   case 35: return 2088;
   case 36: return 2102;
   case 37: return 2117;
   case 38: return 2132;
   case 39: return 2146;
   case 40: return 2161;
   case 41: return 2176;
   case 42: return 2192;
   case 43: return 2207;
   case 44: return 2222;
   case 45: return 2238;
   case 46: return 2253;
   case 47: return 2269;
   case 48: return 2285;
   case 49: return 2301;
   case 50: return 2317;
   case 51: return 2333;
   case 52: return 2349;
   case 53: return 2365;
   case 54: return 2382;
   case 55: return 2398;
   case 56: return 2415;
   case 57: return 2432;
   case 58: return 2449;
   case 59: return 2466;
   case 60: return 2483;
   case 61: return 2500;
   case 62: return 2518;
   case 63: return 2535;
   case 64: return 2553;
   case 65: return 2570;
   case 66: return 2588;
   case 67: return 2606;
   case 68: return 2624;
   case 69: return 2643;
   case 70: return 2661;
   case 71: return 2680;
   case 72: return 2698;
   case 73: return 2717;
   case 74: return 2736;
   case 75: return 2755;
   case 76: return 2774;
   case 77: return 2793;
   case 78: return 2813;
   case 79: return 2832;
   case 80: return 2852;
   case 81: return 2872;
   case 82: return 2892;
   case 83: return 2912;
   case 84: return 2932;
   case 85: return 2953;
   case 86: return 2973;
   case 87: return 2994;
   case 88: return 3015;
   case 89: return 3036;
   case 90: return 3057;
   case 91: return 3078;
   case 92: return 3100;
   case 93: return 3121;
   case 94: return 3143;
   case 95: return 3165;
   case 96: return 3187;
   case 97: return 3209;
   case 98: return 3231;
   case 99: return 3254;
   case 100: return 3276;
   case 101: return 3312;
   case 102: return 3349;
   case 103: return 3386;
   case 104: return 3424;
   case 105: return 3461;
   case 106: return 3500;
   case 107: return 3538;
   case 108: return 3577;
   case 109: return 3617;
   case 110: return 3657;
   case 111: return 3697;
   case 112: return 3738;
   case 113: return 3779;
   case 114: return 3821;
   case 115: return 3863;
   case 116: return 3906;
   case 117: return 3949;
   case 118: return 3993;
   case 119: return 4037;
   case 120: return 4082;
   case 121: return 4127;
   case 122: return 4172;
   case 123: return 4218;
   case 124: return 4265;
   case 125: return 4312;
   case 126: return 4360;
   case 127: return 4408;
   case 128: return 4457;
   case 129: return 4506;
   case 130: return 4556;
   case 131: return 4606;
   case 132: return 4657;
   case 133: return 4708;
   case 134: return 4760;
   case 135: return 4813;
   case 136: return 4866;
   case 137: return 4920;
   case 138: return 4974;
   case 139: return 5029;
   case 140: return 5085;
   case 141: return 5141;
   case 142: return 5198;
   case 143: return 5255;
   case 144: return 5313;
   case 145: return 5372;
   case 146: return 5431;
   case 147: return 5491;
   case 148: return 5552;
   case 149: return 5613;
   case 150: return 5675;
   case 151: return 5738;
   case 152: return 5801;
   case 153: return 5865;
   case 154: return 5930;
   case 155: return 5996;
   case 156: return 6062;
   case 157: return 6129;
   case 158: return 6196;
   case 159: return 6265;
   case 160: return 6334;
   case 161: return 6404;
   case 162: return 6475;
   case 163: return 6546;
   case 164: return 6619;
   case 165: return 6692;
   case 166: return 6766;
   case 167: return 6841;
   case 168: return 6916;
   case 169: return 6993;
   case 170: return 7070;
   case 171: return 7148;
   case 172: return 7227;
   case 173: return 7307;
   case 174: return 7387;
   case 175: return 7469;
   case 176: return 7552;
   case 177: return 7635;
   case 178: return 7719;
   case 179: return 7805;
   case 180: return 7891;
   case 181: return 7978;
   case 182: return 8066;
   case 183: return 8155;
   case 184: return 8245;
   case 185: return 8336;
   case 186: return 8428;
   case 187: return 8522;
   case 188: return 8616;
   case 189: return 8711;
   case 190: return 8807;
   case 191: return 8904;
   case 192: return 9003;
   case 193: return 9102;
   case 194: return 9203;
   case 195: return 9304;
   case 196: return 9407;
   case 197: return 9511;
   case 198: return 9616;
   case 199: return 9722;
   case 200: return 9830;
 };
 return 3276; // never get here
}


unsigned short audio_delaytable(unsigned char c) {
 switch (c) {
   case 0: return 8;
   case 1: return 8;
   case 2: return 8;
   case 3: return 8;
   case 4: return 8;
   case 5: return 8;
   case 6: return 8;
   case 7: return 8;
   case 8: return 8;
   case 9: return 8;
   case 10: return 8;
   case 11: return 8;
   case 12: return 8;
   case 13: return 8;
   case 14: return 8;
   case 15: return 8;
   case 16: return 8;
   case 17: return 8;
   case 18: return 8;
   case 19: return 8;
   case 20: return 8;
   case 21: return 8;
   case 22: return 8;
   case 23: return 8;
   case 24: return 8;
   case 25: return 8;
   case 26: return 8;
   case 27: return 8;
   case 28: return 8;
   case 29: return 8;
   case 30: return 8;
   case 31: return 8;
   case 32: return 8;
   case 33: return 8;
   case 34: return 8;
   case 35: return 8;
   case 36: return 8;
   case 37: return 8;
   case 38: return 8;
   case 39: return 8;
   case 40: return 8;
   case 41: return 8;
   case 42: return 8;
   case 43: return 8;
   case 44: return 8;
   case 45: return 8;
   case 46: return 8;
   case 47: return 8;
   case 48: return 8;
   case 49: return 8;
   case 50: return 8;
   case 51: return 8;
   case 52: return 8;
   case 53: return 8;
   case 54: return 8;
   case 55: return 8;
   case 56: return 8;
   case 57: return 8;
   case 58: return 8;
   case 59: return 8;
   case 60: return 8;
   case 61: return 8;
   case 62: return 8;
   case 63: return 8;
   case 64: return 8;
   case 65: return 8;
   case 66: return 8;
   case 67: return 8;
   case 68: return 8;
   case 69: return 8;
   case 70: return 8;
   case 71: return 8;
   case 72: return 8;
   case 73: return 8;
   case 74: return 8;
   case 75: return 8;
   case 76: return 8;
   case 77: return 8;
   case 78: return 8;
   case 79: return 8;
   case 80: return 8;
   case 81: return 8;
   case 82: return 8;
   case 83: return 8;
   case 84: return 8;
   case 85: return 8;
   case 86: return 8;
   case 87: return 8;
   case 88: return 8;
   case 89: return 8;
   case 90: return 8;
   case 91: return 8;
   case 92: return 8;
   case 93: return 8;
   case 94: return 8;
   case 95: return 8;
   case 96: return 8;
   case 97: return 8;
   case 98: return 8;
   case 99: return 8;
   case 100: return 17;
   case 101: return 18;
   case 102: return 19;
   case 103: return 19;
   case 104: return 20;
   case 105: return 21;
   case 106: return 21;
   case 107: return 22;
   case 108: return 23;
   case 109: return 23;
   case 110: return 24;
   case 111: return 25;
   case 112: return 26;
   case 113: return 26;
   case 114: return 27;
   case 115: return 28;
   case 116: return 28;
   case 117: return 29;
   case 118: return 30;
   case 119: return 30;
   case 120: return 31;
   case 121: return 32;
   case 122: return 33;
   case 123: return 33;
   case 124: return 34;
   case 125: return 35;
   case 126: return 35;
   case 127: return 36;
   case 128: return 37;
   case 129: return 38;
   case 130: return 38;
   case 131: return 39;
   case 132: return 40;
   case 133: return 40;
   case 134: return 41;
   case 135: return 42;
   case 136: return 42;
   case 137: return 43;
   case 138: return 44;
   case 139: return 45;
   case 140: return 45;
   case 141: return 46;
   case 142: return 47;
   case 143: return 47;
   case 144: return 48;
   case 145: return 49;
   case 146: return 49;
   case 147: return 50;
   case 148: return 51;
   case 149: return 52;
   case 150: return 52;
   case 151: return 53;
   case 152: return 54;
   case 153: return 54;
   case 154: return 55;
   case 155: return 56;
   case 156: return 57;
   case 157: return 57;
   case 158: return 58;
   case 159: return 59;
   case 160: return 59;
   case 161: return 60;
   case 162: return 61;
   case 163: return 61;
   case 164: return 62;
   case 165: return 63;
   case 166: return 64;
   case 167: return 64;
   case 168: return 65;
   case 169: return 66;
   case 170: return 66;
   case 171: return 67;
   case 172: return 68;
   case 173: return 68;
   case 174: return 69;
   case 175: return 70;
   case 176: return 71;
   case 177: return 71;
   case 178: return 72;
   case 179: return 73;
   case 180: return 73;
   case 181: return 74;
   case 182: return 75;
   case 183: return 76;
   case 184: return 76;
   case 185: return 77;
   case 186: return 78;
   case 187: return 78;
   case 188: return 79;
   case 189: return 80;
   case 190: return 80;
   case 191: return 81;
   case 192: return 82;
   case 193: return 83;
   case 194: return 83;
   case 195: return 84;
   case 196: return 85;
   case 197: return 85;
   case 198: return 86;
   case 199: return 87;
   case 200: return 88;
 };
 return 17; // never get here
}

// Used to reset the data buffer on BUFFER_EMPTY events
void CALLBACK variosound_waveOutEventCB(WAVE_OUT_EVENT variosound_waveOutEvent)
{
  int i;
  EnterCriticalSection(&CritSec_VarioSound);

  switch (variosound_waveOutEvent)
    {
    case WAVE_OUT_EVENT_NONE:
      break;
    case WAVE_OUT_EVENT_BUFFER_EMPTY:
      variosound_waveOut.ResetBuffer();
      for (i=0; i<BCOUNT; i++) {
	variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ*CHANNELS);
	VarioSound_synthesiseSound();
      }
      break;
    case WAVE_OUT_EVENT_BUFFER_PLAYED:
      variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ*CHANNELS);
      VarioSound_synthesiseSound();

      break;
    default:
      break;
    }

  LeaveCriticalSection(&CritSec_VarioSound);
}


#define sgn(f) (f>0? 1:-1)

extern int iround(double i);

extern void VarioSound_sndparam_int();

extern "C" {

  VARIOSOUND_API void VarioSound_SoundParam() {
    VarioSound_sndparam_int();
  }

  VARIOSOUND_API void VarioSound_SetVAlt(short v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    variosound_valt = max(-100,min(v,100));
    LeaveCriticalSection(&CritSec_VarioSoundV);

  }

  VARIOSOUND_API void VarioSound_SetSTFMode(bool v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    audiofilter.stfmode = v;
    LeaveCriticalSection(&CritSec_VarioSoundV);

  }

  VARIOSOUND_API void VarioSound_SetV(short v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    // copy last value
    variosound_vscale_last = variosound_vscale_in;

    // set new value, clipped
    variosound_vscale_in = (short)max(-100,min(v+1.3*randomgaussian(),100));

    // calculate time elapsed since last sound,
    // usually this will be 500 or 1000 ms
    DWORD fpsTimeLast = fpsTimeSound;
    fpsTimeSound = ::GetTickCount(); // time now
    fpsTimeDelta = (short)(fpsTimeSound-fpsTimeLast);

#ifdef DEBUGAUDIO
    variosound_vscale_timer = true;
#endif

    variosound_vav_in = (9*variosound_vav_in+1*variosound_vscale_in)/10;
    LeaveCriticalSection(&CritSec_VarioSoundV);
  }


  VARIOSOUND_API void VarioSound_SetVdead(short v) {

    EnterCriticalSection(&CritSec_VarioSound);

    audio_deadband_hi = 100+v/2;
    audio_deadband_low = 100-v/2;
    // TODO feature: set sound deadband

    LeaveCriticalSection(&CritSec_VarioSound);
  }


  VARIOSOUND_API void VarioSound_Init() {

    InitializeCriticalSection(&CritSec_VarioSound);
	InitializeCriticalSection(&CritSec_VarioSoundV);  // added sgi

#ifdef DISABLEAUDIO
	return;
#endif

    variosound_waveOut.Init(variosound_waveOutEventCB,
			    THREAD_PRIORITY_TIME_CRITICAL,
			    BCOUNT,
			    BSIZE*FREQZ*CHANNELS,
			    CHANNELS,
			    FREQ,
			    8);

    EnterCriticalSection(&CritSec_VarioSound);

    VarioSound_SetVdead(0);
    VarioSound_SetV(0);
    VarioSound_EnableSound(false);

    variosound_waveOut.StartThread();
    variosound_sound = TRUE;
    int i;
    for (i=0; i<BCOUNT; i++) {
      VarioSound_synthesiseSound();
      variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ*CHANNELS);
    }
    LeaveCriticalSection(&CritSec_VarioSound);
  }

//
// Modified to use BOOL instead of bool as the parameter.  RB
// Changed back! JG
//
  VARIOSOUND_API void VarioSound_EnableSound(bool sound) {
    EnterCriticalSection(&CritSec_VarioSound);
    variosound_sound = sound;
    if (sound) {
      variosound_waveOut.ResumeThread();
    } else {
      variosound_waveOut.SuspendThread();
    }
    LeaveCriticalSection(&CritSec_VarioSound);
    //	variosound_sound = !variosound_sound;
  }


  VARIOSOUND_API void VarioSound_Close(void) {  // added sgi
#ifndef DISABLEAUDIO
    variosound_waveOut.StopThread();   // added manually stop the thread
#endif
    DeleteCriticalSection(&CritSec_VarioSound);
    DeleteCriticalSection(&CritSec_VarioSoundV);
  }

  VARIOSOUND_API void VarioSound_SetSoundVolume(int v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    variosound_waveOut.SetSoundVolume(v);
    LeaveCriticalSection(&CritSec_VarioSoundV);
  }

}

// Audio volume controls

bool suppressdeadband = false;

void audio_soundmode_volume(void) {
  short grad;

  // calculate gradient/urgency based volume
  if (audiofilter.stfmode) {
    grad = abs(audiofilter.vstfsmooth);
    audio_volume = (audio_volume*6+2*max(1, 1+min(7*grad/80, 7)))/8;
  } else {
    grad = abs(audiofilter.vcur-audiofilter.vsmoothlast);
    audio_volume = (audio_volume*6+2*max(3, 3+min(grad/8, 5)))/8;
  }
  // if gradient/urgency is high, suppress deadband
  if (audio_volume>5) {
    suppressdeadband = true;
  } else {
    suppressdeadband = false;
  }
}


void audio_soundmode_smooth(short vinst, short vstf) {
  audiofilter.vstfsmoothlast = audiofilter.vstfsmooth;
  audiofilter.vstfsmooth = (7*audiofilter.vstfsmooth+1*vstf)/8;
  audiofilter.vsmoothlast = audiofilter.vsmooth;
  audiofilter.vsmooth = max(0, min((4*audiofilter.vsmooth+4*vinst)/8, 200));
  audiofilter.vcur = max(0, min(vinst,200));
}


void audio_soundmode_cruise(void) {
  // apply deadbands and sound types
  // if too slow, type=double beep
  // if too fast, type=continuous
  // if in deadband zone, silence

  // sound tone equal to speed command:
  // if too slow, pitch is low (like sink tone)
  // if too fast, pitch is high (like climb tone)

  short vthis;

  audio_soundtype = 0; // quiet

  if ((audiofilter.vstfsmooth>1)||(audiofilter.vstfsmooth< -1)) {
    // only make sounds required if speed is more than 10% too fast
    // or 5% too slow
    // (this is speed-to-fly deadband)

    vthis = max(0, min(100+audiofilter.vstfsmooth,200));

    if (audiofilter.vstfsmooth>0) { // slow down

      audio_soundtype = 2; // long beeps
      audio_beepfrequency = audio_delaytable(10); // long period
      audio_soundfrequency = audio_frequencytable((unsigned char)vthis);

    } else
    if (audiofilter.vstfsmooth<0) { // speed up

      audio_soundtype = 4; // double beep
      audio_beepfrequency = audio_delaytable(10); // long period
      audio_soundfrequency = audio_frequencytable((unsigned char)vthis);

    }
  }

  // this disables alternate sound
  audio_altsoundfrequency = audio_soundfrequency;

}


void audio_soundmode_climb(void) {
  // apply deadbands and sound types
  // if in lift, and above deadband or deadband suppressed, type=short beep
  // if in sink, and below deadband or deadband suppressed, type=continuous
  // if in deadband zone, silence

  // main vario sound frequency based on instantaneous reading
  audio_soundfrequency = audio_frequencytable((unsigned char)audiofilter.vcur);

  // beep frequency based on smoothed input to prevent jumps
  audio_beepfrequency = audio_delaytable((unsigned char)audiofilter.vsmooth);

  // this disables alternate sound
  audio_altsoundfrequency = audio_soundfrequency;

  if (audiofilter.vcur>= audio_deadband_hi) {
    audio_soundtype = 1;
  } else if (suppressdeadband && (audiofilter.vsmooth>=100)) {
    audio_soundtype = 1;
  } else if (audiofilter.vcur<= audio_deadband_low) {
    audio_soundtype = 3;
  } else if (suppressdeadband && (audiofilter.vsmooth<100)) {
    audio_soundtype = 3;
  } else {
    audio_soundtype = 0;
  }

}


void audio_soundmode_stall(short delta) {
  // stall warning sound is hi-lo fast beep, range of sound based on
  // magnitude of stall speed difference

  short mag = max(20,min(100,delta*5));

  audio_volume = 8; // make it loud, maybe this should be based on mag

  audio_beepfrequency = audio_delaytable(130); // shortish period

  audio_soundfrequency = audio_frequencytable((unsigned char)(100-mag));
  audio_altsoundfrequency = audio_frequencytable((unsigned char)(100+mag));
  audio_soundtype = 5; // even beep
}

#include "externs.h"

void audio_soundmode(short vinst, short vstf) {

  audio_soundmode_smooth(vinst, vstf);

  audio_soundmode_volume();

  // apply sounds
  short stalldelta = 0; // MACCREADY*10;
  if (stalldelta>0) {
    audio_soundmode_stall(stalldelta);
  } else {
    if (audiofilter.stfmode) {
      audio_soundmode_cruise();
    } else {
      audio_soundmode_climb();
    }
  }

}

void VarioSound_sndparam_int() {

  // this is called by the wave thread when filling the buffer
  // since the buffer is filled after copying the previous buffer,
  // it should really calculate the sound at the time of the completion
  // of the current buffer.  This could be computed by looking at the
  // sound period

  EnterCriticalSection(&CritSec_VarioSoundV);

  // find position of this sound in the slice
  DWORD	fpsTime0 = ::GetTickCount();
  short delta = (short)(fpsTime0-fpsTimeSound);
  delta = min(max(1,delta), fpsTimeDelta);

  // tp_delay/FREQZ

  // find smoothed input (linearly interpolated between last and this sound)

  variosound_vav = variosound_vav_in;

#ifdef AUDIOSMOOTHING
  variosound_vscale = (short)((
    variosound_vscale_in*(delta)
    +variosound_vscale_last*(fpsTimeDelta-delta)
    )/fpsTimeDelta);
#else
  variosound_vscale = variosound_vscale_in;
#endif
  variosound_vscale = max(-100,min(100,variosound_vscale));

#ifdef DEBUGAUDIO
  // for debugging, find time between sending a command and the time the sound gets generated
  if (variosound_vscale_timer) {
    DWORD	fpsTime0 = ::GetTickCount();

    int dt = (fpsTime0-fpsTimeSound);
    variosound_vscale_timer = false;

    static int kaverage = 0;
    static int dtave = 0;
    kaverage++;
    dtave+= dt;
    if (kaverage % 100 == 0) {
      LogDebug(_T("sound delay %d ms\r\n"), dtave/kaverage);
      dtave=0;
      kaverage=0;
    }
  }
#endif

  // compute sound parameters

  audio_soundparameters();
  audio_soundmode(variosound_vscale+100, variosound_valt);

  LeaveCriticalSection(&CritSec_VarioSoundV);

}



void VarioSound_synthesiseSound() {
  unsigned char *buf = (unsigned char*)variosound_buffer;
  unsigned char *endBuf = (unsigned char*)variosound_buffer+BSIZE*FREQZ*CHANNELS;
  unsigned char bsound;


#ifdef DEBUGAUDIO
  DWORD	fpsTime0 = ::GetTickCount();
#endif

  //

  while (buf < endBuf) {

    bsound = audio_get_sound_byte();

    *buf = bsound;
	buf++;
        if (CHANNELS>1) {
          *buf = bsound;
          buf++;
        }

  }
}

