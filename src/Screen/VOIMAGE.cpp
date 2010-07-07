//-------------------------------------------------------------------
// VOImage implementation
//-------------------------------------------------------------------
//
// Copyright ©2000 Virtual Office Systems Incorporated
// All Rights Reserved
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is
// not sold for profit without the authors written consent, and
// providing that this notice and the authors name is included.
//
// This code can be compiled, modified and distributed freely, providing
// that this copyright information remains intact in the distribution.
//
// This code may be compiled in original or modified form in any private
// or commercial application.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage, in any form, caused
// by this code. Use it at your own risk.
//-------------------------------------------------------------------

/*
NOTE: Some portions copyright as above

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

#include "Screen/VOIMAGE.hpp"

// Construction/Destruction

HDC		CVOImage::g_hdc;
int		CVOImage::g_iScale = 100;
int		CVOImage::g_iMaxWidth = 10000;
int		CVOImage::g_iMaxHeight = 10000;
BOOL	CVOImage::g_bStretchBlt = FALSE;

CVOImage::CVOImage()
{
	m_hbitmap = 0;
}

CVOImage::~CVOImage()
{
	if(m_hbitmap)
		DeleteObject(m_hbitmap);
}

BOOL CVOImage::Load(HDC hdc, const TCHAR *pcszFileName)
{
	if(m_hbitmap)
		DeleteObject(m_hbitmap);

	if(!g_hdc)
		g_hdc = CreateCompatibleDC(hdc);

	HRESULT hr;
	BYTE    szBuffer[1024] = {0};
	HANDLE hFile = INVALID_HANDLE_VALUE;

	DecompressImageInfo	dii;

	hFile = CreateFile(pcszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	// Fill in the 'DecompressImageInfo' structure
	dii.dwSize = sizeof( DecompressImageInfo );		// Size of this structure
	dii.pbBuffer = szBuffer;						// Pointer to the buffer to use for data
	dii.dwBufferMax = 1024;							// Size of the buffer
	dii.dwBufferCurrent = 0;						// The amount of data which is current in the buffer
	dii.phBM = &m_hbitmap;							// Pointer to the bitmap returned (can be NULL)
	dii.ppImageRender = NULL;						// Pointer to an IImageRender object (can be NULL)
	dii.iBitDepth = GetDeviceCaps(hdc,BITSPIXEL);	// Bit depth of the output image
	dii.lParam = ( LPARAM ) hFile;					// User parameter for callback functions
	dii.hdc = g_hdc;								// HDC to use for retrieving palettes
	dii.iScale = g_iScale;							// Scale factor (1 - 100)
	dii.iMaxWidth = g_iMaxWidth;					// Maximum width of the output image
	dii.iMaxHeight = g_iMaxHeight;					// Maxumum height of the output image
	dii.pfnGetData = GetImageData;					// Callback function to get image data
	dii.pfnImageProgress = ImageProgress;			// Callback function to notify caller of progress decoding the image
	dii.crTransparentOverride = ( UINT ) -1;		// If this color is not (UINT)-1, it will override the
													// transparent color in the image with this color. (GIF ONLY)

	// Process and decompress the image data
	hr = DecompressImageIndirect( &dii );

	// Clean up
	CloseHandle( hFile );

	BITMAP	bmp;

	GetObject(m_hbitmap, sizeof(BITMAP), &bmp);

	m_dwWidth = bmp.bmWidth;
	m_dwHeight = bmp.bmHeight;

	return TRUE;
}

BOOL CVOImage::Draw(HDC hdc, int x, int y, int cx, int cy)
{
	BITMAP	bmp;
	HGDIOBJ	hOldBitmap;

	g_bStretchBlt = !(cx == -1 && cy == -1);
	hOldBitmap = SelectObject(g_hdc, m_hbitmap);
	GetObject(m_hbitmap, sizeof(BITMAP), &bmp);

	if (g_bStretchBlt)
	{
	  // Stretch to fit
	  StretchBlt(hdc, x , y, cx, cy, g_hdc,0,0,
		     bmp.bmWidth,bmp.bmHeight,SRCCOPY );
	}
	else
	{
	  BitBlt(hdc, x, y, bmp.bmWidth,
		 bmp.bmHeight, g_hdc,0,0,SRCCOPY );
	}

	SelectObject(g_hdc, hOldBitmap);

	return TRUE;
}

DWORD CALLBACK CVOImage::GetImageData(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam )
{
	DWORD dwNumberOfBytesRead;

	if ( (HANDLE)lParam == INVALID_HANDLE_VALUE )
		return 0;

	ReadFile( (HANDLE)lParam, szBuffer, dwBufferMax, &dwNumberOfBytesRead, (OVERLAPPED *)NULL);

	// Return number of bytes read
	return dwNumberOfBytesRead;
}

void CALLBACK CVOImage::ImageProgress(IImageRender *pRender, BOOL bComplete, LPARAM lParam )
{
	(void)lParam;
	(void)pRender;
	if( bComplete )
	{
		;// (Optional) add code here for completion processing
	}
}
