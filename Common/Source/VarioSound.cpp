// VarioSound.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "VarioSound.h"
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



////////////////////

#include "WaveThread.h"

CWaveOutThread variosound_waveOut;

#define BSIZE 10
#define BCOUNT 4

#define LOWFI
#ifdef LOWFI
#define FREQZ 80
#define FREQ 8000
#endif
#ifdef MEDFI
#define FREQZ 220
#define FREQ 22050
#endif
#ifdef HIFI
#define FREQZ 441
#define FREQ 44100
#endif
unsigned char  variosound_buffer[BSIZE*FREQZ];
short variosound_sin[256];
short variosound_sinquiet[256];

// 20ms 8000Hz, Mono, 16bit
unsigned char variosound_volumescale[201];
int variosound_freqtable[201];
int variosound_delaytable[201];
short variosound_volume;

#include <math.h>

#define fbase 400 // base frequency
#define noct 4
#define vmax 10.0
#define tzero 75
#define delaymult 5.0
double koct = log(noct)/log(2);

double variosound_vscale_in =0.0;
double variosound_vav_in = 0.0;

double variosound_vscale=0.0;
double variosound_vcur = 0.0;
double variosound_vav=0.0;
bool variosound_sound = true;

int tp_delay = 50;
int tp_sound = 20;
int tp_avsound = 20;
#define QUANT 16

char variosound_pvbuf[201*256];
char variosound_pvbufq[201*256];

short quantisesound(double vv) {
	short k;
	k = (short)(vv*100)+100;
	if (k>200) 
		return 100;
	if (k<0)
		return 0;
	return k;
}

CRITICAL_SECTION  CritSec_VarioSound;
CRITICAL_SECTION  CritSec_VarioSoundV;


void VarioSound_sndparam() {
  EnterCriticalSection(&CritSec_VarioSoundV);
  
  variosound_vscale = variosound_vscale_in;
  variosound_vav = variosound_vav_in;

  LeaveCriticalSection(&CritSec_VarioSoundV);

  double vtp = variosound_vscale;

  // FREQZ*tzero = number of samples

  variosound_vcur = //0.8*variosound_vscale+0.2*variosound_vcur;
    variosound_vscale;
  //  variosound_vav = 0.9*variosound_vav+0.1*variosound_vcur;

  vtp = variosound_vcur;

  tp_delay = variosound_delaytable[quantisesound(vtp)];

  ///
  tp_sound = variosound_freqtable[quantisesound(variosound_vcur)];

  int i;
  i= (tp_delay/tp_sound);
  tp_delay = i*tp_sound;

  tp_avsound = variosound_freqtable[quantisesound(variosound_vav)];
  variosound_volume = quantisesound(variosound_vcur);
}


/////////////////////////////////


char f_sound_loud(unsigned long i) {
  short phase;
  phase = (short)((i/tp_sound) % 256);
  return variosound_pvbuf[variosound_volume*256+phase];
}

 
char f_sound_quiet(unsigned long i) {
  short phase;
  phase = (short)((i/tp_avsound) % 256);
  return variosound_pvbufq[variosound_volume*256+phase];
}


unsigned long f_quiet_next(bool isquiet) {
  if (variosound_vcur>0) {
    if (isquiet) {
      return ((long)tp_delay*63);
    } else {
      return ((long)tp_delay*255);
    }
  } else {
    if (isquiet) {
      return ((long)tp_delay*191);
    } else {
      return ((long)tp_delay*255);
    }
  }
}


void VarioSound_synthesiseSound() {
  char *buf = (char*)variosound_buffer;
  char *endBuf = (char*)variosound_buffer+BSIZE*FREQZ;
  static unsigned long idelay=0;
  static unsigned long ii=0;
  static bool mode_quiet = true;
  static unsigned long inext= 0;

  while (buf < endBuf) {

    if (idelay >= inext) {

      // get next transition count

      VarioSound_sndparam();

      inext = f_quiet_next(mode_quiet);
      idelay = 0;

      // transition mode

      mode_quiet = !mode_quiet;
    }

    if (!variosound_sound) {

      while ((buf< endBuf) && (idelay < inext)) {

	*buf = (unsigned char)0x80;
	buf++;

	idelay+= 256;
	ii+= QUANT*256;
      }

    } else if (mode_quiet) {

      while ((buf< endBuf) && (idelay < inext)) {

	// do quiet stuff
	//	if (variosound_vcur<0) {
	//	  *buf = f_sound_loud(ii); // sink is solid tone
	//	} else {
	  *buf = f_sound_quiet(ii);
	  //	}
	buf++;

	idelay+= 256;

	ii+= QUANT*256;
      }

    } else {

      while ((buf< endBuf) && (idelay < inext)) {

	// do noisy stuff

	*buf = f_sound_loud(ii);
	//	f_sound_loud(ii);
	buf++;

	idelay+= 256;

	ii+= QUANT*256;

      }

    }
  }

}


//////////////////////////////////


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
	variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ);
	VarioSound_synthesiseSound();
      }
      break;
    case WAVE_OUT_EVENT_BUFFER_PLAYED:
      variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ);
      VarioSound_synthesiseSound();
		
      break;
    default:
      break;
    }

  LeaveCriticalSection(&CritSec_VarioSound);
}


#define sgn(f) (f>0? 1:-1)

///////////////////////////////////////////

extern int iround(double i);

extern "C" {


  VARIOSOUND_API void VarioSound_SetV(short v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    variosound_vscale_in = v/100.0;
    if (variosound_vscale_in>1.0) {
      variosound_vscale_in = 1.0;
    }
    if (variosound_vscale_in<-1.0) {
      variosound_vscale_in = -1.0;
    }
    variosound_vav_in = 0.9*variosound_vav_in+0.1*variosound_vscale_in;
    LeaveCriticalSection(&CritSec_VarioSoundV);
  }


  VARIOSOUND_API void VarioSound_SetVdead(short v) {
    EnterCriticalSection(&CritSec_VarioSound);
    int i;
    double vv;

    if (v==0) {
      for (i=0; i<201; i++) {
	variosound_volumescale[i]= 10;
      }
    } else {
      for (i=0; i<201; i++) {
	vv = (i-100)/(double)v;
	if (vv<0) {
	  variosound_volumescale[i] = iround(7*(1.0-1.0/(vv*vv+1)));
	} else {
	  variosound_volumescale[i]= iround(10*(1.0-1.0/(vv*vv+1)));
	}
      }
    }

    for (i=0; i<201; i++) {
      for (int j=0; j<256; j++) {
	variosound_pvbuf[i*256+j] = 
	  (variosound_sin[j]*variosound_volumescale[i])/16+128;
	variosound_pvbufq[i*256+j] = 
	  (variosound_sinquiet[j]*variosound_volumescale[i])/16+128;
      }
    }

    LeaveCriticalSection(&CritSec_VarioSound);
  }


  VARIOSOUND_API void VarioSound_Init() {

    InitializeCriticalSection(&CritSec_VarioSound);
	InitializeCriticalSection(&CritSec_VarioSoundV);  // added sgi

    variosound_waveOut.Init(variosound_waveOutEventCB,
			    // THREAD_PRIORITY_TIME_CRITICAL, 
			    THREAD_PRIORITY_ABOVE_NORMAL, 
			    BCOUNT,
			    BSIZE*FREQZ,
			    1,
			    FREQ,
			    8);

    EnterCriticalSection(&CritSec_VarioSound);
    int i;
    for (i=0; i<256; i++) {
      double f = sin(i*3.14159*2.0/256);
     // f = sqrt(fabs(f))*sgn(f);
      variosound_sin[i]= (short)iround(64.0*f);
      variosound_sinquiet[i]= (short)iround(16.0*f);
    }

    for (i=0; i<201; i++) {
      double vv = (i-100)/100.0;
      variosound_freqtable[i] = (int)((QUANT*FREQ)*pow(2,-vv*koct)/fbase);
      if (vv>=0.0) {
	variosound_delaytable[i] = (int)(FREQZ*tzero/(1+fabs(vv)*5));
      } else {
	variosound_delaytable[i] = (int)(FREQZ*tzero);
      }
    }

    VarioSound_SetVdead(0);
    VarioSound_SetV(0);
    VarioSound_EnableSound(false);
 
    variosound_waveOut.StartThread();
    variosound_sound = true;
    for (i=0; i<BCOUNT; i++) {
      VarioSound_synthesiseSound();
      variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ);
    }
    LeaveCriticalSection(&CritSec_VarioSound);
  }

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
    variosound_waveOut.StopThread();   // added manually stop the thread
    DeleteCriticalSection(&CritSec_VarioSound);
    DeleteCriticalSection(&CritSec_VarioSoundV);
  }

  VARIOSOUND_API void VarioSound_SetSoundVolume(int v) {
    EnterCriticalSection(&CritSec_VarioSoundV);
    variosound_waveOut.SetSoundVolume(v);
    LeaveCriticalSection(&CritSec_VarioSoundV);
  }

}


extern HINSTANCE                       hInst; // The current instance

BOOL PlayResource (LPTSTR lpName) 
{ 
  BOOL bRtn; 
  LPTSTR lpRes; 
  HANDLE hResInfo, hRes; 

  // Find the wave resource.
  hResInfo = FindResource (hInst, lpName, TEXT("WAVE")); 

  if (hResInfo == NULL) 
    return FALSE; 

  // Load the wave resource. 
  hRes = LoadResource (hInst, (HRSRC)hResInfo); 

  if (hRes == NULL) 
    return FALSE; 

  // Lock the wave resource and play it. 
  lpRes = (LPTSTR)LockResource ((HGLOBAL)hRes);
  
  if (lpRes != NULL) 
    { 
    bRtn = sndPlaySound (lpRes, SND_MEMORY | SND_ASYNC | SND_NODEFAULT ); 
    } 
  else 
    bRtn = 0; 

  return bRtn; 
}



/////////////////////////// Audio volume controls

