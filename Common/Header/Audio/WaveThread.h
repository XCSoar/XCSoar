/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#if !defined(AFX_WAVETHREAD_H__C76FCF03_9877_4C5E_92BA_DEDB09884315__INCLUDED_)
#define AFX_WAVETHREAD_H__C76FCF03_9877_4C5E_92BA_DEDB09884315__INCLUDED_

#include <mmsystem.h>

#define INTERNAL_WAVEOUT_BUFFER_COUNT 2   // This is one part of the wave output delay
                                          //  if you use e.g. 20ms buffers the delay
                                          //  INTERNAL_WAVEOUT_BUFFER_COUNT * 20ms

// Used for waveIn callback function
typedef enum _WAVE_IN_EVENT
{
   WAVE_IN_EVENT_NONE         = 0,
   WAVE_IN_EVENT_NEW_BUFFER   = 1,
   WAVE_IN_EVENT_BUFFER_FULL  = 2,
} WAVE_IN_EVENT;

// Used for waveOut callback function
typedef enum _WAVE_OUT_EVENT
{
   WAVE_OUT_EVENT_NONE           = 0,
   WAVE_OUT_EVENT_BUFFER_PLAYED  = 1,
   WAVE_OUT_EVENT_BUFFER_EMPTY   = 2,
} WAVE_OUT_EVENT;

// Declaration of used callback functions
typedef void (CALLBACK* cbWaveIn)(WAVE_IN_EVENT);
typedef void (CALLBACK* cbWaveOut)(WAVE_OUT_EVENT);


// CWaveInThread thread
class CWaveInThread
{
public:
	CWaveInThread();
	~CWaveInThread();

   BOOL Init(cbWaveIn pcbWaveIn,
             DWORD dwActiveThreadPriority,
             WORD wBufferCount,
             DWORD dwSingleBufferSize,
             WORD wChannels,
             DWORD dwSamplesPerSec,
             WORD wBitsPerSample);

   BOOL StartThread();
   BOOL StopThread();

   void ResetBuffer();
   BOOL ReadData(unsigned char* pBuffer, DWORD dwCount);
   DWORD GetBytesPerSecond();
   DWORD GetDataLen();

protected:

   static DWORD WINAPI ThreadProc(LPVOID lpParameter);

   BOOL              m_bInitialized;
   DWORD             m_threadId;
   HANDLE            m_thread;
   CRITICAL_SECTION  m_critSecRtp;
   HANDLE            m_hEventKill;
   DWORD             m_dwActiveThreadPriority;
   WORD              m_wBufferCount;
   DWORD             m_dwSingleBufferSize;
   DWORD             m_dwCopyBufferBytes;
   unsigned char    *m_pCopyBufferPos;
   DWORD             m_dwBufferSize;
   unsigned char    *m_pWaveInBuffer;
   unsigned char    *m_pWaveCopyBuffer;
   HWAVEIN           m_hWaveIn;
   WAVEHDR          *m_pWaveHeaderArray;
   BOOL              m_bPause;
   WAVEFORMATEX      m_wfx;
   cbWaveIn          m_pcbWaveIn;
};

// CWaveOutThread thread
class CWaveOutThread
{
public:
	CWaveOutThread();
	~CWaveOutThread();

   BOOL Init(cbWaveOut pcbWaveOut,
             DWORD dwActiveThreadPriority,
             WORD wBufferCount,
             DWORD dwSingleBufferSize,
             WORD wChannels,
             DWORD dwSamplesPerSec,
             WORD wBitsPerSample);

   BOOL StartThread();
   BOOL StopThread();
   void ResetBuffer();
   BOOL WriteData(unsigned char* pBuffer, DWORD dwCount);
   DWORD GetBytesPerSecond();
   DWORD GetDataLen();
   void SuspendThread();
   void ResumeThread();
   void SetSoundVolume(int volpercent);
   void RestoreSoundVolume();
   DWORD dwVolume_restore;


protected:

   static DWORD WINAPI ThreadProc(LPVOID lpParameter);

   BOOL              m_bInitialized;
   DWORD             m_threadId;
   HANDLE            m_thread;
   CRITICAL_SECTION  m_critSecRtp;
   HANDLE            m_hEventKill;
   DWORD             m_dwActiveThreadPriority;
   WORD              m_wBufferCount;
   DWORD             m_dwSingleBufferSize;
   DWORD             m_dwCopyBufferBytes;
   unsigned char    *m_pCopyBufferPos;
   DWORD             m_dwBufferSize;
   unsigned char    *m_pWaveOutBuffer;
   unsigned char    *m_pWaveCopyBuffer;
   HWAVEOUT          m_hWaveOut;
   int               m_idWaveOut;
   WAVEHDR          *m_pWaveHeaderArray;
   BOOL              m_bPause;
   WAVEFORMATEX      m_wfx;
   cbWaveOut         m_pcbWaveOut;
   DWORD             m_dwUsedWaveOutBuffers;
   WORD              m_wNextCopyBuffer;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAVETHREAD_H__C76FCF03_9877_4C5E_92BA_DEDB09884315__INCLUDED_)
