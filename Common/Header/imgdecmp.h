/*---------------------------------------------------------------------------*\
 *
 * (c) Copyright Microsoft Corp. 1997-98 All Rights Reserved
 *
 *  module: imgdecmp.h
 *  date:
 *  author: jaym
 *
 *  purpose: 
 *
\*---------------------------------------------------------------------------*/
#ifndef __IMGDECMP_H__
#define __IMGDECMP_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "imgrendr.h"

typedef void (CALLBACK *PROGRESSFUNC)(IImageRender *pRender, BOOL bComplete, LPARAM lParam);
typedef DWORD (CALLBACK *GETDATAFUNC)(LPSTR szBuffer, DWORD dwBufferMax, LPARAM lParam);

typedef struct tagDecompressImageInfo {
	DWORD                   dwSize;                                 // Size of this structure
	LPBYTE                  pbBuffer;                               // Pointer to the buffer to use for data
	DWORD                   dwBufferMax;                    // Size of the buffer
	DWORD                   dwBufferCurrent;                // The amount of data which is current in the buffer
	HBITMAP *               phBM;                                   // Pointer to the bitmap returned (can be NULL)
	IImageRender ** ppImageRender;                  // Pointer to an IImageRender object (can be NULL)
	int                             iBitDepth;                              // Bit depth of the output image
	LPARAM                  lParam;                                 // User parameter for callback functions
	HDC                             hdc;                                    // HDC to use for retrieving palettes
	int                             iScale;                                 // Scale factor (1 - 100)
	int                             iMaxWidth;                              // Maximum width of the output image
	int                             iMaxHeight;                             // Maxumum height of the output image
	GETDATAFUNC             pfnGetData;                             // Callback function to get more data
	PROGRESSFUNC    pfnImageProgress;               // Callback function to notify caller of progress decoding the image
	COLORREF                crTransparentOverride;  // If this color is not (UINT)-1, it will override the
											// transparent color in the image with this color. (GIF ONLY)
} DecompressImageInfo;

#define IMGDECOMP_E_NOIMAGE             0x800b0100

COLORREF *
GetHalftonePalette();

COLORREF *
Get332Palette();

HRESULT
DecompressImageIndirect(DecompressImageInfo *pParams);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !__IMGDECMP_H__
