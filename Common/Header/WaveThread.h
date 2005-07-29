// WaveThread.h : header file
//


#if !defined(AFX_WAVETHREAD_H__C76FCF03_9877_4C5E_92BA_DEDB09884315__INCLUDED_)
#define AFX_WAVETHREAD_H__C76FCF03_9877_4C5E_92BA_DEDB09884315__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


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


/////////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////////
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
