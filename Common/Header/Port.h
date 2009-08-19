#if !defined(AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_PORT_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <windows.h>

#define NMEA_BUF_SIZE 100

// Forward declaration
struct DeviceDescriptor_t;

class ComPort {
 public:
	ComPort(struct DeviceDescriptor_t *d);
	~ComPort() { };

	void PutChar(BYTE);
	void WriteString(const TCHAR *);
	void Flush();

	BOOL Initialize(LPCTSTR, DWORD);
	BOOL Close();

	int SetRxTimeout(int);
	unsigned long SetBaudrate(unsigned long);

	BOOL StopRxThread();
	BOOL StartRxThread();
	void ProcessChar(char);

	int GetChar();
	int Read(void *Buffer, size_t Size);

 private:
	static DWORD WINAPI ThreadProc(LPVOID);
	DWORD ReadThread();

	HANDLE hPort;
	HANDLE hReadThread;
	DWORD dwMask;
	TCHAR sPortName[8];
	BOOL CloseThread;
	BOOL fRxThreadTerminated;

	TCHAR BuildingString[NMEA_BUF_SIZE];
	int bi;
	struct DeviceDescriptor_t *dev;
};

#endif
