// VarioSound.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "VarioSound.h"
#include <math.h>
#include "XCSoar.h"

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

#define BSIZE 5
#define BCOUNT INTERNAL_WAVEOUT_BUFFER_COUNT

#define MEDFI
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
#define nocthi 3
#define noctlo 1
#define tzero 75

short variosound_vscale_in = 0;
short variosound_vav_in = 0;

short variosound_vscale=0;
short variosound_vscale_last=0;
short variosound_vcur = 0;
short variosound_vav=0;
BOOL variosound_sound = TRUE;

int tp_delay = 50;
int tp_sound = 20;
int tp_avsound = 20;
#define QUANT 16

char variosound_pvbuf[201*256];
char variosound_pvbufq[201*256];

#ifdef DEBUG
bool variosound_vscale_timer = false;
#endif
DWORD fpsTimeSound = 0;
short fpsTimeDelta = 0;

short quantisesound(short vv) {
  return max(0,min(200,vv+100));
}

CRITICAL_SECTION  CritSec_VarioSound;
CRITICAL_SECTION  CritSec_VarioSoundV;


void VarioSound_sndparam() {

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

  variosound_vscale = (short)((
    variosound_vscale_in*(delta)
    +variosound_vscale_last*(fpsTimeDelta-delta)
    )/fpsTimeDelta);

  variosound_vscale = max(-100,min(100,variosound_vscale));


#ifdef DEBUG
  if (variosound_vscale_timer) {
    DWORD	fpsTime0 = ::GetTickCount();

    int dt = (fpsTime0-fpsTimeSound);
    variosound_vscale_timer = false;

    static int kaverage = 0;
    static int dtave = 0;
    kaverage++;
    dtave+= dt;
    if (kaverage % 10 == 0) {
      char message[100];
      sprintf(message,"sound delay %d ms\r\n", dtave/kaverage);
      DebugStore(message);
      dtave=0;
      kaverage=0;
    }
  }
#endif

  LeaveCriticalSection(&CritSec_VarioSoundV);

  variosound_vcur = variosound_vscale;
  // (7*variosound_vscale+3*variosound_vcur)/10;

  short qs = quantisesound(variosound_vcur);

  tp_delay = variosound_delaytable[qs];
  tp_sound = variosound_freqtable[qs];

  int i;
  i= (tp_delay/tp_sound);
  tp_delay = i*tp_sound;

  tp_avsound = tp_sound; // variosound_freqtable[quantisesound(variosound_vav)];
  variosound_volume = qs;
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
      return ((long)tp_delay*254); // JMW was 191
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

#ifdef DEBUG
  DWORD	fpsTime0 = ::GetTickCount();
#endif


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

        *buf = f_sound_quiet(ii);
	buf++;

	idelay+= 256;

	ii+= QUANT*256;
      }

    } else {

      while ((buf< endBuf) && (idelay < inext)) {

	// do noisy stuff

	*buf = f_sound_loud(ii);
	buf++;

	idelay+= 256;

	ii+= QUANT*256;

      }

    }
  }

#ifdef DEBUG
  int dfpsTime = ::GetTickCount()-fpsTime0;
  static int kaverage = 0;
  static int dtave = 0;
  static int timethis = 0;
  kaverage++;
  dtave+= dfpsTime;
  if (kaverage % 100 == 0) {

    char message[100];
    int dtbig = (::GetTickCount() - timethis)/kaverage;
    timethis = fpsTime0;
    sprintf(message,"dt sound %d ns %d ms\r\n", dtave*1000/kaverage,
            dtbig);
    DebugStore(message);
    dtave=0;
    kaverage=0;

  }

#endif

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
    // copy last value
    variosound_vscale_last = variosound_vscale_in;

    // set new value, clipped
    variosound_vscale_in = max(-100,min(v,100));

    // calculate time elapsed since last sound,
    // usually this will be 500 or 1000 ms
    DWORD fpsTimeLast = fpsTimeSound;
    fpsTimeSound = ::GetTickCount(); // time now
    fpsTimeDelta = (short)(fpsTimeSound-fpsTimeLast);

#ifdef DEBUG
    variosound_vscale_timer = true;
#endif

    variosound_vav_in = (9*variosound_vav_in+1*variosound_vscale_in)/10;
    LeaveCriticalSection(&CritSec_VarioSoundV);
  }


  VARIOSOUND_API void VarioSound_SetVdead(short v) {

    if (v == 0)
      return;

    EnterCriticalSection(&CritSec_VarioSound);
    int i;
    double vv;

    if (v<=0) {
      for (i=0; i<201; i++) {
        vv = (i-100)/(double)v;
        if (vv<0) {
          variosound_volumescale[i] = 7;
        } else {
	         variosound_volumescale[i]= 10;
       	}
      }
    } else {
      for (i=0; i<201; i++) {
        vv = (i-100)/(double)v;
        if (vv<0) {
          variosound_volumescale[i] = iround(3*(1.0-1.0/(vv*vv+1)));
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
			    THREAD_PRIORITY_TIME_CRITICAL,
			    BCOUNT,
			    BSIZE*FREQZ,
			    1,
			    FREQ,
			    8);

    EnterCriticalSection(&CritSec_VarioSound);
    int i;
    for (i=0; i<256; i++) {
      double f = sin(i*3.14159*2.0/256);
      variosound_sin[i]= (short)iround(64.0*f);
      variosound_sinquiet[i]= (short)iround(8.0*f);
    }

    double kocthi = log(nocthi)/log(2);
    double koctlo = log(noctlo)/log(2);

    for (i=0; i<201; i++) {
      double vv = (i-100)/100.0;
      if (vv>=0.0) {
        variosound_freqtable[i] = (int)((QUANT*FREQ)*pow(2,-vv*kocthi)/fbase);
	variosound_delaytable[i] = (int)(FREQZ*tzero/(1+fabs(vv)*5));
      } else {
	variosound_delaytable[i] = (int)(FREQZ*tzero); // JMW was 1
        variosound_freqtable[i] = (int)((QUANT*FREQ)*pow(2,-vv*koctlo)/fbase);
      }
    }

    VarioSound_SetVdead(0);
    VarioSound_SetV(0);
    VarioSound_EnableSound(false);

    variosound_waveOut.StartThread();
    variosound_sound = TRUE;
    for (i=0; i<BCOUNT; i++) {
      VarioSound_synthesiseSound();
      variosound_waveOut.WriteData(variosound_buffer, BSIZE*FREQZ);
    }
    LeaveCriticalSection(&CritSec_VarioSound);
  }
//
// Modified to use BOOL instead of bool as the parameter.  RB
//
  VARIOSOUND_API void VarioSound_EnableSound(BOOL sound) {
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

