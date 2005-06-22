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

#include "stdafx.h"

#include "VOImage.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

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

BOOL CVOImage::Load(HDC hdc, LPCTSTR pcszFileName)
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

HBITMAP CVOImage::Copy()
{
	BITMAP	bm, bmNew;
	HBITMAP hNew;

	SelectObject(g_hdc, m_hbitmap);

	::GetObject(m_hbitmap, sizeof(BITMAP), &bm);

	HDC hdc = CreateCompatibleDC(g_hdc);
	hNew = CreateCompatibleBitmap(g_hdc, bm.bmWidth, bm.bmHeight);
	SelectObject(hdc, hNew);

	if(BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, g_hdc, 0, 0, SRCCOPY))
	{
		HBITMAP hPrev = (HBITMAP) ::GetObject(hNew, sizeof(BITMAP), &bmNew);
			
		::SelectObject(hdc, hPrev);
	}

	DeleteDC(hdc);
	return hNew;
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
		StretchBlt(hdc, x , y, cx, cy, g_hdc,0,0,bmp.bmWidth,bmp.bmHeight,SRCCOPY );
	}
	else
	{
		BitBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, g_hdc,0,0,SRCCOPY );
	}

	SelectObject(g_hdc, hOldBitmap);

	return TRUE;
}

DWORD CVOImage::GetHeight()
{
	return m_dwHeight;
}

DWORD CVOImage::GetWidth()
{
	return m_dwWidth;
}

BOOL CVOImage::SetBitmap(HDC hdc, DWORD dwResourceID, LPCTSTR pcszClass, HMODULE hModule)
{
	if(!g_hdc)
		g_hdc = CreateCompatibleDC(hdc);

	HRESULT hr;
	BYTE    szBuffer[1024] = {0}; 
	DecompressImageInfo	dii;

	CVOResource	res(hModule, dwResourceID, pcszClass);

	if(!res.IsLoaded())
		return FALSE;

	res.SetUserData(0);	// Use this for the current resource offset

	// Fill in the 'DecompressImageInfo' structure
	dii.dwSize = sizeof( DecompressImageInfo );		// Size of this structure
	dii.pbBuffer = szBuffer;						// Pointer to the buffer to use for data
	dii.dwBufferMax = 1024;							// Size of the buffer
	dii.dwBufferCurrent = 0;						// The amount of data which is current in the buffer
	dii.phBM = &m_hbitmap;							// Pointer to the bitmap returned (can be NULL)
	dii.ppImageRender = NULL;						// Pointer to an IImageRender object (can be NULL)
	dii.iBitDepth = GetDeviceCaps(hdc,BITSPIXEL);	// Bit depth of the output image
	dii.lParam = ( LPARAM ) &res;					// User parameter for callback functions
	dii.hdc = g_hdc;								// HDC to use for retrieving palettes
	dii.iScale = g_iScale;							// Scale factor (1 - 100)
	dii.iMaxWidth = g_iMaxWidth;					// Maximum width of the output image
	dii.iMaxHeight = g_iMaxHeight;					// Maxumum height of the output image
	dii.pfnGetData = GetImageResourceData;			// Callback function to get image data
	dii.pfnImageProgress = ImageProgress;			// Callback function to notify caller of progress decoding the image
	dii.crTransparentOverride = ( UINT ) -1;		// If this color is not (UINT)-1, it will override the
																								// transparent color in the image with this color. (GIF ONLY)
	// Process and decompress the image data
	hr = DecompressImageIndirect( &dii );
		
	BITMAP	bmp;

	GetObject(m_hbitmap, sizeof(BITMAP), &bmp);

	m_dwWidth = bmp.bmWidth;
	m_dwHeight = bmp.bmHeight;

	return TRUE;
}

DWORD CALLBACK CVOImage::GetImageData(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam )
{
	DWORD dwNumberOfBytesRead;

	if ( (HANDLE)lParam == INVALID_HANDLE_VALUE )
		return 0;

	ReadFile( (HANDLE)lParam, szBuffer, dwBufferMax, &dwNumberOfBytesRead, NULL );

	// Return number of bytes read
	return dwNumberOfBytesRead;
}

DWORD CALLBACK CVOImage::GetImageResourceData(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam)
{
	DWORD			dwNumberOfBytesToRead = dwBufferMax;
	CVOResource*	pRes = (CVOResource*) lParam;

	if(!pRes)
		return 0;

	DWORD			dwResourceOffset = pRes->GetUserData();

	if(dwResourceOffset + dwNumberOfBytesToRead > pRes->GetSize() )
		dwNumberOfBytesToRead = pRes->GetSize() - dwResourceOffset;

	memmove(szBuffer, pRes->GetData() + dwResourceOffset, dwNumberOfBytesToRead);

	pRes->SetUserData(dwResourceOffset + dwNumberOfBytesToRead);
	return dwNumberOfBytesToRead;	// return amount read
}

void CALLBACK CVOImage::ImageProgress(IImageRender *pRender, BOOL bComplete, LPARAM lParam )
{
	if( bComplete )
	{
		;// (Optional) add code here for completion processing
	}
}

BOOL CVOImage::IsLoaded()
{
	return (m_hbitmap != 0);
}

CVOResource::CVOResource(HMODULE hModule, DWORD dwResourceID, LPCTSTR pcszClass)
{
	m_dwSize = 0;
	m_hGlobal = 0;
	m_pData = 0;

	m_hrsrc = FindResource(hModule, (LPCWSTR)dwResourceID, pcszClass);

	if(m_hrsrc == 0)
		return;

	m_dwSize = SizeofResource(hModule, m_hrsrc);
	m_hGlobal = LoadResource(hModule, m_hrsrc);
	m_pData = (PBYTE) LockResource(m_hGlobal);
}

CVOResource::~CVOResource()
{
	if(m_hGlobal)
		DeleteObject(m_hGlobal);
}

BOOL CVOResource::IsLoaded()
{
	return (m_pData != NULL);
}
