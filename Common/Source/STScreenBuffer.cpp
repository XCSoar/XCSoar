
/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "STScreenBuffer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int CSTScreenBuffer::CorrectedWidth(int nWidth)
{
	return ( ( nWidth + 3 ) / 4 ) * 4;
}

struct DIBINFO : public BITMAPINFO
{
	RGBQUAD	 arColors[255];    // Color table info - adds an extra 255 entries to palette

	operator LPBITMAPINFO()          { return (LPBITMAPINFO) this; }
	operator LPBITMAPINFOHEADER()    { return &bmiHeader;          }
	RGBQUAD* ColorTable()            { return bmiColors;           }
};


/////////////////////////////////////////////////////////////////////////////
// CSTScreenBuffer

CSTScreenBuffer::CSTScreenBuffer()
	: m_hBitmap(NULL),
	  m_pBuffer(NULL),
	  m_pBufferTmp(NULL),
	  m_pDC(NULL),
	  memDc(NULL)
{
}

CSTScreenBuffer::~CSTScreenBuffer()
{
	if (m_hBitmap!=NULL) {
		ReleaseDC();
		::DeleteObject(m_hBitmap);
	}
	if (m_pBufferTmp) {
	  free(m_pBufferTmp);
	}
	if (memDc) {
	  DeleteDC(memDc); memDc = NULL;
	}
}

BOOL CSTScreenBuffer::CreateBitmap(int nWidth, int nHeight)
{
  ASSERT(nWidth>0);
  ASSERT(nHeight>0);
  
  if (m_hBitmap!=NULL) DeleteObject(m_hBitmap);
  
  m_nCorrectedWidth = CorrectedWidth(nWidth);
  m_nWidth = nWidth;
  m_nHeight = nHeight;
  
  DIBINFO  dibInfo;
  
  dibInfo.bmiHeader.biBitCount = 24;
  dibInfo.bmiHeader.biClrImportant = 0;
  dibInfo.bmiHeader.biClrUsed = 0;
  dibInfo.bmiHeader.biCompression = 0;
  dibInfo.bmiHeader.biHeight = m_nHeight;
  dibInfo.bmiHeader.biPlanes = 1;
  dibInfo.bmiHeader.biSize = 40;
  dibInfo.bmiHeader.biSizeImage = m_nCorrectedWidth*m_nHeight*3;
  dibInfo.bmiHeader.biWidth = m_nCorrectedWidth;
  dibInfo.bmiHeader.biXPelsPerMeter = 3780;
  dibInfo.bmiHeader.biYPelsPerMeter = 3780;
  dibInfo.bmiColors[0].rgbBlue = 0;
  dibInfo.bmiColors[0].rgbGreen = 0;
  dibInfo.bmiColors[0].rgbRed = 0;
  dibInfo.bmiColors[0].rgbReserved = 0;
  
  HDC hDC = ::GetDC(NULL);
  ASSERT(hDC);
  m_hBitmap = CreateDIBSection(hDC, (const BITMAPINFO*)dibInfo, DIB_RGB_COLORS, (void**)&m_pBuffer, NULL, 0);
  ::ReleaseDC(NULL, hDC);
  ASSERT(m_hBitmap);
  ASSERT(m_pBuffer);
  
  m_pBufferTmp = (BGRColor*)malloc(sizeof(BGRColor)*m_nHeight*m_nCorrectedWidth);
  
  return TRUE;
}

void CSTScreenBuffer::Create(int nWidth, int nHeight)
{
	ASSERT(nWidth>0);
	ASSERT(nHeight>0);

	CreateBitmap(nWidth, nHeight);
}

void CSTScreenBuffer::Create(int nWidth, int nHeight, COLORREF clr)
{
	ASSERT(nWidth>0);
	ASSERT(nHeight>0);

	CreateBitmap(nWidth, nHeight);

	BGRColor bgrColor = BGRColor(GetBValue(clr), GetGValue(clr), GetRValue(clr));
	int nPosition = 0;

	for (int y=0; y<nHeight; y++) {
		nPosition = m_nCorrectedWidth*y;
		for (int x=0; x<nWidth; x++) {
			m_pBuffer[nPosition] = bgrColor;
			nPosition++;
		}
	}
}

void CSTScreenBuffer::Create(HBITMAP hBitmap)
{
  BITMAP bm;
  GetObject(hBitmap, sizeof(BITMAP), &bm);
  CreateBitmap(bm.bmWidth, bm.bmHeight);
  
  HDC targetDc;
  memDc = CreateCompatibleDC(NULL);
  targetDc = CreateCompatibleDC(NULL);
  
  HBITMAP hOldBitmap1 = (HBITMAP)::SelectObject(memDc, hBitmap);
  HBITMAP hOldBitmap2 = (HBITMAP)::SelectObject(targetDc, m_hBitmap);
  
  BitBlt(targetDc, 0, 0, bm.bmWidth, bm.bmHeight, memDc, 0, 0, SRCCOPY);
  
  ::SelectObject(memDc, hOldBitmap1);
  ::SelectObject(targetDc, hOldBitmap2);
  DeleteDC(memDc); memDc = NULL;
  DeleteDC(targetDc);
}

void CSTScreenBuffer::Create(HDC *pDC, RECT rect)
{
  ASSERT(m_pDC);

  CreateBitmap(rect.right-rect.left, rect.bottom-rect.top);
  BitBlt(m_pDC, 0,0, rect.right-rect.left, rect.bottom-rect.top, 
	 *pDC, rect.left, rect.top, SRCCOPY);
}

void CSTScreenBuffer::CreateRGB(void *pData, int nWidth, int nHeight)
{
  ASSERT(pData);
  ASSERT(nWidth>0);
  ASSERT(nHeight>0);
  
  CreateBitmap(nWidth, nHeight);
  
  byte *pByteData = (byte*)pData;
  int nPosition = 0;
  int nDataPosition = 0;
  
  for (int y=0; y<nHeight; y++) {
    nPosition = m_nCorrectedWidth*(m_nHeight-y-1);
    nDataPosition = nWidth*3*y;
    for (int x=0; x<nWidth; x++) {
      m_pBuffer[nPosition].m_R = pByteData[nDataPosition++];
      m_pBuffer[nPosition].m_G = pByteData[nDataPosition++];
      m_pBuffer[nPosition].m_B = pByteData[nDataPosition++];
      nPosition++;
    }
  }
}

BOOL CSTScreenBuffer::Draw(HDC* pDC, POINT ptDest)
{
  ASSERT(m_hBitmap);
  ReleaseDC();
  
  POINT Origin = {0,0};
  
  BOOL bResult = FALSE;
  
  if (!memDc) {
    memDc = CreateCompatibleDC(*pDC);
  }
  if (!memDc) {
    return FALSE;
  }
  
  HBITMAP m_hOldBitmap = (HBITMAP)::SelectObject(memDc, m_hBitmap);
  bResult = BitBlt(*pDC, ptDest.x, ptDest.y, 
		   m_nWidth, m_nHeight, 
		   memDc, 
		   Origin.x, Origin.y, SRCCOPY);
  ::SelectObject(memDc, m_hOldBitmap);
  //  DeleteDC(memDc); memDc = NULL;
  
  return bResult;
}

BOOL CSTScreenBuffer::DrawStretch(HDC* pDC, RECT rcDest)
{
  POINT ptDest;
  int cx;
  int cy;

  ptDest.x = rcDest.left;
  ptDest.y = rcDest.top;
  cx = rcDest.right-rcDest.left;
  cy = rcDest.bottom-rcDest.top;
  return DrawStretch(pDC, ptDest, cx, cy);
}


BOOL CSTScreenBuffer::DrawStretch(HDC* pDC, POINT ptDest, int cx, int cy)
{
  ASSERT(m_hBitmap);
  ReleaseDC();
  
  POINT Origin = {0,0};
  
  BOOL bResult = FALSE;
  
  if (!memDc) {
    memDc = CreateCompatibleDC(*pDC);
  }
  if (!memDc) {
    return FALSE;
  }

  HBITMAP m_hOldBitmap = (HBITMAP)::SelectObject(memDc, m_hBitmap);

  int cropsize = m_nHeight*cx/cy;
  
  bResult = StretchBlt(*pDC, ptDest.x, ptDest.y, 
		       cx, cy, memDc, 
		       Origin.x, Origin.y, 
		       cropsize, m_nHeight, SRCCOPY);

  ::SelectObject(memDc, m_hOldBitmap);
  //  DeleteDC(memDc); memDc = NULL;
  
  return bResult;
}


HBITMAP CSTScreenBuffer::CreateBitmapByRGBArray(void *pData, int nWidth, int nHeight)
{
  HBITMAP hResult = NULL;	
  CSTScreenBuffer sb;
  
  sb.CreateRGB(pData, nWidth, nHeight);
  hResult = sb.m_hBitmap;
  
  sb.m_hBitmap = NULL;
  sb.m_pBuffer = NULL;
  
  return hResult;
}

HDC CSTScreenBuffer::GetDC()
{
  if (m_pDC) return m_pDC;

  m_pDC = CreateCompatibleDC(NULL);
  if (!m_pDC) {
    return NULL;
  }
  
  m_hSaveBitmap = (HBITMAP)SelectObject(m_pDC,GetHBitmap());
  return m_pDC;
}

void CSTScreenBuffer::ReleaseDC()
{
  if (m_pDC) {
    SelectObject(m_pDC, m_hSaveBitmap);
    DeleteDC(m_pDC);
    m_pDC = NULL;
  }
}

#define getindy(x,y) m_pBuffer[m_nCorrectedWidth*(y)+x]

void CSTScreenBuffer::Smooth()
{
  short r,g,b;
  int i, i0;
  int ic;

  int ix, iy;

  for (iy = 0; iy< m_nHeight; iy++) {
    for (ix = 0; ix< m_nCorrectedWidth; ix++) {

      ic = 2;

      i0 = m_nCorrectedWidth*iy+ix;
      i = i0;

      r = m_pBuffer[i0].m_R*ic;
      g = m_pBuffer[i0].m_G*ic;
      b = m_pBuffer[i0].m_B*ic;

      if (ix<m_nCorrectedWidth-1) {
	i0 = i+1;
	r += m_pBuffer[i0].m_R;
	g += m_pBuffer[i0].m_G;
	b += m_pBuffer[i0].m_B;
	ic++;
      }

      if (ix>1) {
 	i0 = i-1;
	r += m_pBuffer[i0].m_R;
	g += m_pBuffer[i0].m_G;
	b += m_pBuffer[i0].m_B;
	ic++;
      }

      if (iy<m_nHeight-1) {
 	i0 = i+m_nCorrectedWidth;
	r += m_pBuffer[i0].m_R;
	g += m_pBuffer[i0].m_G;
	b += m_pBuffer[i0].m_B;
	ic++;
      }

      if (iy>1) {
 	i0 = i-m_nCorrectedWidth;
	r += m_pBuffer[i0].m_R;
	g += m_pBuffer[i0].m_G;
	b += m_pBuffer[i0].m_B;
	ic++;
      }

      m_pBufferTmp[i].m_R = r/ic;
      m_pBufferTmp[i].m_G = g/ic;
      m_pBufferTmp[i].m_B = b/ic;

    }
  }
 
  // copy it back to main buffer
  for (i=0; i<m_nCorrectedWidth*m_nHeight; i++) {
    m_pBuffer[i] = m_pBufferTmp[i];
  }

}


void CSTScreenBuffer::Smooth2()
{

  unsigned short r,g,b;
  unsigned short ic;
  /*  
  unsigned short kk[5][5] = {{0,0,1,0,0},
			     {0,2,3,2,0},
			     {1,3,6,3,1},
			     {0,2,3,2,0},
			     {0,0,1,0,0}};
  unsigned short kk[3][3] = {{1,2,1},
			     {2,3,2},
			     {1,2,1}};
  */
			     
  static unsigned short kk[25] =  {0,1,2,1,0,
				   1,2,3,2,1,
				   2,3,4,3,2,
				   1,2,3,2,1,
				   0,1,2,1,0};
  static unsigned int kdelta[25];
#define KSIZE 5
#define KOFFS 2 
#define KSIZE2 25  

  short ix, iy;
  short iix, iiy;
  int ik;

  BGRColor *col;

  static bool initialised = false;

  int i=0;

  if (!initialised) {
    for (iiy= 0; iiy<KSIZE; iiy++) {
      for (iix= 0; iix<KSIZE; iix++) {
	ix = iix-KOFFS;
	iy = iiy-KOFFS;
	kdelta[i] = m_nCorrectedWidth*iy+ix;
	i++;
      }
    }
    initialised = true;
  } 

  BGRColor* mpb_out = m_pBufferTmp;
  BGRColor* mpb_in = m_pBuffer;

  for (iy = 0; iy< m_nHeight; ++iy) {
    for (ix = 0; ix< m_nCorrectedWidth; ++ix) {

      r = 0;
      g = 0;
      b = 0;
      ic = 0;

      i=0;
      for (iiy= -KOFFS; iiy<=KOFFS; ++iiy) {
	for (iix= -KOFFS; iix<=KOFFS; ++iix) {

	  short dx = iix+ix;
	  short dy = iiy+iy;
	  if ((dx>0)&&(dx< m_nCorrectedWidth)&&
	      (dy>0)&&(dy< m_nHeight)) {

	    ik = kk[i];

	    if (ik) {
	      col = mpb_in+kdelta[i];
	      r += ik*col->m_R;
	      g += ik*col->m_G;
	      b += ik*col->m_B;
	      ic += ik;
	    }
	  }
	  i++;
	}
      }

      mpb_out->m_R = r/ic;
      mpb_out->m_G = g/ic;
      mpb_out->m_B = b/ic;
      mpb_out++;
      mpb_in++;
    }
  }
 
  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp, 
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}



void CSTScreenBuffer::Quantise()
{
  BGRColor* mpb = m_pBuffer;
  BGRColor* mpbtop = m_pBuffer+m_nCorrectedWidth*m_nHeight;
 
  for (mpb= m_pBuffer; mpb<mpbtop; mpb++) {
    mpb->m_R |= (0x07);
    mpb->m_G |= (0x07);
    mpb->m_B |= (0x07);
  }

}

// 1 + 2 + 4 = 7

void CSTScreenBuffer::Zoom(int step) {
  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor* dst_start = m_pBufferTmp;

  int smallx = m_nCorrectedWidth/step;
  int smally = m_nHeight/step;
  int j, x, y;
  int rowsize = m_nCorrectedWidth*sizeof(BGRColor);

  for (y= (smally-1); y>=0; y--) {
    dst = m_pBufferTmp+m_nCorrectedWidth*(y*step);
    dst_start = dst;
    for (x=0;x< smallx; x++) {
      for (j=0; j<step; j++) {
	*dst = *src;
	dst++;  
      }
      src++;
    }
    // done first row, now copy each row
    for (j=1; j<step; j++) {
      memcpy((char*)dst, (char*)dst_start, rowsize);
      dst+= m_nCorrectedWidth;
    }
  }

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp, 
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}


void CSTScreenBuffer::HorizontalBlur(int boxw) {

  int muli = (boxw*2+1);
  int mul;
  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor *c;

  for (int y=0;y< m_nHeight;y++)
    {
      int tot_r=0;
      int tot_g=0;
      int tot_b=0;
      for (int x=0;x<boxw;x++) {
	tot_r+= src[x].m_R;
	tot_g+= src[x].m_G;
	tot_b+= src[x].m_B;
      }
      for (x=0;x< m_nCorrectedWidth; x++)
	{
	  mul = muli;
	  if (x>boxw) {
	    c = src-boxw-1;
	    tot_r-= c->m_R;
	    tot_g-= c->m_G;
	    tot_b-= c->m_B;
	  }  else {
	    mul -= (boxw-x);
	  }
	  if (x+boxw< m_nCorrectedWidth) {
	    c = src+boxw;
	    tot_r+= c->m_R;
	    tot_g+= c->m_G;
	    tot_b+= c->m_B;
	  } else {
	    mul -= (boxw-m_nCorrectedWidth+x+1);
	  }
	  dst->m_R=(tot_r/mul);
	  dst->m_G=(tot_g/mul);
	  dst->m_B=(tot_b/mul);

	  src++;
	  dst++;
	}
    }	

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp, 
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}

void CSTScreenBuffer::VerticalBlur(int boxh) {

  int mul;
  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor *c;

  int i;
  int muli = (boxh*2+1);
  int iboxh = m_nCorrectedWidth*boxh;
  for (int x=0;x< m_nCorrectedWidth; x++)
    {
      int tot_r=0;
      int tot_g=0;
      int tot_b=0;
      for (int y=0;y<boxh;y++) {
	i = x+y*m_nCorrectedWidth;
	tot_r+= src[i].m_R;
	tot_g+= src[i].m_G;
	tot_b+= src[i].m_B;
      }
      for (y=0;y< m_nHeight; y++)
	{
	  mul = muli;
	  i = x+y*m_nCorrectedWidth;
	  if (y>boxh) {
	    c = src+i-iboxh-m_nCorrectedWidth;
	    tot_r-= c->m_R;
	    tot_g-= c->m_G;
	    tot_b-= c->m_B;
	  }  else {
	    mul -= (boxh-y);
	  }
	  if (y+boxh< m_nHeight) {
	    c = src+i+iboxh;
	    tot_r+= c->m_R;
	    tot_g+= c->m_G;
	    tot_b+= c->m_B;
	  } else {
	    mul -= (boxh-m_nHeight+y+1);
	  }
	  dst[i].m_R=(tot_r)/mul;
	  dst[i].m_G=(tot_g)/mul;
	  dst[i].m_B=(tot_b)/mul;
	}
    }	

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp, 
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}
