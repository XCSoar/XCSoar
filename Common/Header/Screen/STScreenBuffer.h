/////////////////////////////////////////////////////////////////////////////
// File name:      STScreenBuffer.h
// Author:         Vassili Philippov (vasja@spbteam.com)
// Created:        June 2001
// Last changed:   07 August 2001
// Version:        2.0
// Description:    Class for pixel working with images. One could get access
//                 to image bits using this library or create custom image.

#if !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
#define AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
// BGRColor structure encapsulates color information about one point. Color
// order is Blue, Green, Red (not RGB).

struct BGRColor
{
	BGRColor() {}
	BGRColor(unsigned char R, unsigned char G, unsigned char B)
		: m_R(R), m_G(G), m_B(B) {}
	unsigned char m_B;
	unsigned char m_G;
	unsigned char m_R;
};


/////////////////////////////////////////////////////////////////////////////
// CSTScreenBuffer class provides fast drawing methods and offscreen buffer.

class CSTScreenBuffer
{
public:
	// Creates uninitialized buffer. Call Create or CreateRGB to
	// initialize the buffer.
	CSTScreenBuffer();

	virtual ~CSTScreenBuffer();

	// Creates buffer with the given size and fills it with
	// the given color
	void Create(int nWidth, int nHeight, COLORREF clr);

	// Draws buffer into the given device context starting from
	// the given point (top left corner)
	BOOL DrawStretch(HDC *pDC, POINT ptDest, unsigned int cx,
                         unsigned int cy);

	HDC memDc;

	// Draws buffer into given device context within rectangle
	BOOL DrawStretch(HDC* pDC, RECT rcDest);

	// Sets color of the given point
	inline void SetPoint(int nX, int nY, unsigned char R,
                             unsigned char G, unsigned char B) {
	  // m_pBuffer[m_nCorrectedWidth*(m_nHeight-nY-1)+nX] = BGRColor(R,G,B);
	  m_pBuffer[m_nCorrectedWidth*(nY)+nX] = BGRColor(R,G,B);
	}

	inline void SetPoint(int i, unsigned char R,
                             unsigned char G, unsigned char B) {
	  m_pBuffer[i] = BGRColor(R,G,B);
	}

	inline void SetPoint(int i, const BGRColor& c) {
	  m_pBuffer[i] = c;
	}
        BGRColor *GetBuffer(void) {
          return m_pBuffer;
        }

	void HorizontalBlur(unsigned int boxw);
	void VerticalBlur(unsigned int boxh);
	void Zoom(unsigned int step);

	// Returns color of the given point
	inline BGRColor GetPoint(int nX, int nY) {
	  //		return m_pBuffer[m_nCorrectedWidth*(m_nHeight-nY-1)+nX];
	  return m_pBuffer[m_nCorrectedWidth*(nY)+nX];
	}

	// Returns real width of the screen buffer. It could be slightly more then
	// requested width. This paramater is important only when you work with
	// points array directly (using GetPointsArray function).
	int GetCorrectedWidth() {
		return m_nCorrectedWidth;
	}

	// Returns screen buffer width.
	int GetWidth() {
		return m_nWidth;
	}

	// Returns screen buffer height.
	int GetHeight() {
		return m_nHeight;
	}

public:
	// Returns minimum width that is greater then the given width and
	// that is acceptable as image width (not all numbers are acceptable)
	static int CorrectedWidth(int nWidth);

protected:
	// Creates internal bitmap and image buffer. Assignes width and
	// height properties
	BOOL CreateBitmap(int nWidth, int nHeight);

	unsigned int m_nWidth;
	unsigned int m_nHeight;
	unsigned int m_nCorrectedWidth;
	BGRColor *m_pBuffer;
	BGRColor *m_pBufferTmp;
	HBITMAP m_hBitmap;

	// Members related to device context
	HBITMAP m_hSaveBitmap;
};

#endif // !defined(AFX_STSCREENBUFFER_H__22D62F5D_32E2_4785_B3D9_2341C11F84A3__INCLUDED_)
