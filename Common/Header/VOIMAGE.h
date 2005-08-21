//-------------------------------------------------------------------
// VOImage Header File
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

#if !defined(AFX_VOIMAGE_H__B83C4202_DB1E_48BE_92A5_21019F9EE6FC__INCLUDED_)
#define AFX_VOIMAGE_H__B83C4202_DB1E_48BE_92A5_21019F9EE6FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "imgdecmp.h"

class CVOResource
{
public:
	CVOResource(HMODULE hModule, DWORD dwResourceID, LPCTSTR pcszClass);
	~CVOResource();

	BOOL IsLoaded();

	DWORD	GetSize()	{ return m_dwSize; }
	PBYTE	GetData()	{ return m_pData; }

	void	SetUserData(DWORD dwValue)	{ m_dwUser = dwValue; }
	DWORD	GetUserData()				{ return m_dwUser; }

protected:
	DWORD m_dwUser;

	DWORD m_dwSize;
	PBYTE m_pData;
	HGLOBAL m_hGlobal;
	HRSRC m_hrsrc;
};

class CVOImage
{
public:
	HBITMAP Copy();
	static DWORD CALLBACK GetImageResourceData(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam);
	BOOL IsLoaded();
	static void CALLBACK ImageProgress( IImageRender *, BOOL, LPARAM);
	static DWORD CALLBACK GetImageData( LPSTR, DWORD, LPARAM);
	BOOL SetBitmap(HDC hdc, DWORD dwResourceID, LPCTSTR pcszClass = TEXT("IMAGE"), HMODULE hModule = 0 );
	DWORD GetWidth();
	DWORD GetHeight();
	BOOL Draw(HDC hdc, int x, int y, int cx = -1, int cy = -1);
	BOOL Load(HDC hdc, LPCTSTR pcszFileName);
	CVOImage();
	virtual ~CVOImage();
	operator HBITMAP() { return m_hbitmap; }
   static int	g_iScale;

protected:
	DWORD		m_dwHeight;
	DWORD		m_dwWidth;
	HBITMAP		m_hbitmap;
	static BOOL g_bStretchBlt;
	static int	g_iMaxHeight;
	static int	g_iMaxWidth;
	//static int	g_iScale;
	static HDC	g_hdc;
};

#endif // !defined(AFX_VOIMAGE_H__B83C4202_DB1E_48BE_92A5_21019F9EE6FC__INCLUDED_)
