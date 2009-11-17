
/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "Screen/STScreenBuffer.h"

#include <assert.h>
#include <stdlib.h>

// Construction/Destruction

int CSTScreenBuffer::CorrectedWidth(int nWidth)
{
	return ( ( nWidth + 3 ) / 4 ) * 4;
}

// CSTScreenBuffer

CSTScreenBuffer::CSTScreenBuffer()
	: m_pBuffer(NULL),
	  m_pBufferTmp(NULL)
{
}

CSTScreenBuffer::~CSTScreenBuffer()
{
  if (m_hBitmap.defined())
    m_hBitmap.reset();

	if (m_pBufferTmp) {
	  free(m_pBufferTmp);
	}
        memDc.reset();
}

BOOL CSTScreenBuffer::CreateBitmap(int nWidth, int nHeight)
{
  assert(nWidth>0);
  assert(nHeight>0);

  if (m_hBitmap.defined())
    m_hBitmap.reset();

  m_nCorrectedWidth = CorrectedWidth(nWidth);
  m_nWidth = nWidth;
  m_nHeight = nHeight;

  m_pBuffer = (BGRColor *)m_hBitmap.create(m_nCorrectedWidth, m_nHeight);
  assert(m_hBitmap.defined());
  assert(m_pBuffer);

  m_pBufferTmp = (BGRColor*)malloc(sizeof(BGRColor)*m_nHeight*m_nCorrectedWidth);

  return TRUE;
}

void CSTScreenBuffer::Create(int nWidth, int nHeight, const Color clr)
{
	assert(nWidth>0);
	assert(nHeight>0);

	CreateBitmap(nWidth, nHeight);

        BGRColor bgrColor = BGRColor(clr.blue(), clr.green(), clr.red());
	int nPosition = 0;

	for (int y=0; y<nHeight; y++) {
		nPosition = m_nCorrectedWidth*y;
		for (int x=0; x<nWidth; x++) {
			m_pBuffer[nPosition] = bgrColor;
			nPosition++;
		}
	}
}

BOOL CSTScreenBuffer::DrawStretch(Canvas &canvas, RECT rcDest)
{
  POINT ptDest;
  unsigned int cx;
  unsigned int cy;

  ptDest.x = rcDest.left;
  ptDest.y = rcDest.top;
  cx = rcDest.right-rcDest.left;
  cy = rcDest.bottom-rcDest.top;
  return DrawStretch(canvas, ptDest, cx, cy);
}

#include "InfoBoxLayout.h"

BOOL CSTScreenBuffer::DrawStretch(Canvas &canvas, POINT ptDest,
                                  unsigned int cx,
                                  unsigned int cy)
{
  assert(m_hBitmap.defined());

  POINT Origin = {0,0};

  if (!memDc.defined()) {
    memDc.set(canvas);
  }
  if (!memDc.defined()) {
    return FALSE;
  }

  memDc.select(m_hBitmap);

  int cropsize;
  if ((cy<m_nWidth)||(InfoBoxLayout::landscape)) {
    cropsize = m_nHeight*cx/cy;
  } else {
    // NOT TESTED!
    cropsize = m_nWidth;
  }

  canvas.stretch(ptDest.x, ptDest.y, cx, cy,
                 memDc, Origin.x, Origin.y, cropsize, m_nHeight);
  /*
  BitBlt(*pDC,
         ptDest.x, ptDest.y,
         cx, cy, memDc, 0, 0, SRCCOPY);
  */

  return true;
}

void CSTScreenBuffer::Zoom(unsigned int step) {
  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor* dst_start = m_pBufferTmp;

  const unsigned int smallx = m_nCorrectedWidth/step;
  const unsigned int smally = m_nHeight/step;
  const unsigned int rowsize = m_nCorrectedWidth*sizeof(BGRColor);
  const unsigned int wstep = m_nCorrectedWidth*step;
  const unsigned int stepmo = step-1;

  dst_start = m_pBufferTmp+(smally-1)*wstep;
  for (unsigned int y= smally; y--; dst_start-= wstep) {
    dst = dst_start;
    for (unsigned int x= smallx; x--; src++) {
      for (unsigned int j= step; j--; ) {
	*dst++ = *src;
      }
    }
    // done first row, now copy each row
    for (unsigned int k= stepmo; k--; dst+= m_nCorrectedWidth) {
      memcpy((char*)dst, (char*)dst_start, rowsize);
    }
  }

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp,
	 rowsize*m_nHeight);

}


void CSTScreenBuffer::HorizontalBlur(unsigned int boxw) {

  const unsigned int muli = (boxw*2+1);
  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor *c;

  const unsigned int off1 = boxw+1;
  const unsigned int off2 = m_nCorrectedWidth-boxw-1;
  const unsigned int right = m_nCorrectedWidth-boxw;

  for (unsigned int y=m_nHeight;y--; )
    {
      unsigned int tot_r=0;
      unsigned int tot_g=0;
      unsigned int tot_b=0;
      unsigned int x;

      c = src+boxw-1;
      for (x=boxw;x--; c--) {
        tot_r+= c->m_R;
        tot_g+= c->m_G;
        tot_b+= c->m_B;
      }
      for (x=0;x< m_nCorrectedWidth; x++)
	{
	  unsigned int acc = muli;
	  if (x>boxw) {
	    c = src-off1;
	    tot_r-= c->m_R;
	    tot_g-= c->m_G;
	    tot_b-= c->m_B;
	  }  else {
	    acc += x-boxw;
	  }
	  if (x< right) {
	    c = src+boxw;
	    tot_r+= c->m_R;
	    tot_g+= c->m_G;
	    tot_b+= c->m_B;
	  } else {
	    acc += off2-x;
	  }
	  dst->m_R=(unsigned char)(tot_r/acc);
	  dst->m_G=(unsigned char)(tot_g/acc);
	  dst->m_B=(unsigned char)(tot_b/acc);

	  src++;
	  dst++;
	}
    }

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp,
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}

void CSTScreenBuffer::VerticalBlur(unsigned int boxh) {

  BGRColor* src = m_pBuffer;
  BGRColor* dst = m_pBufferTmp;
  BGRColor *c, *d, *e;

  const unsigned int muli = (boxh*2+1);
  const unsigned int iboxh = m_nCorrectedWidth*boxh;
  const unsigned int off1 = iboxh+m_nCorrectedWidth;
  const unsigned int off2 = m_nHeight-boxh-1;
  const unsigned int bottom = m_nHeight-boxh;

  for (unsigned int x= m_nCorrectedWidth; x--;)
    {
      unsigned int tot_r=0;
      unsigned int tot_g=0;
      unsigned int tot_b=0;
      unsigned int y;

      c = d = src+x;
      e = dst+x;
      for (y=boxh;y--; c+= m_nCorrectedWidth) {
	tot_r+= c->m_R;
	tot_g+= c->m_G;
	tot_b+= c->m_B;
      }

      for (y=0;y< m_nHeight; y++) {
	unsigned int acc = muli;
        if (y>boxh) {
          c = d-off1;
          tot_r-= c->m_R;
          tot_g-= c->m_G;
          tot_b-= c->m_B;
        }  else {
          acc += y-boxh;
        }
        if (y< bottom) {
          c = d+iboxh;
          tot_r+= c->m_R;
          tot_g+= c->m_G;
          tot_b+= c->m_B;
        } else {
          acc += off2-y;
        }
        e->m_R=(unsigned char)(tot_r/acc);
        e->m_G=(unsigned char)(tot_g/acc);
        e->m_B=(unsigned char)(tot_b/acc);
        d+= m_nCorrectedWidth;
        e+= m_nCorrectedWidth;
      }
    }

  // copy it back to main buffer
  memcpy((char*)m_pBuffer, (char*)m_pBufferTmp,
	 m_nCorrectedWidth*m_nHeight*sizeof(BGRColor));

}
