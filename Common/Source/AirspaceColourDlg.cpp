#include "stdafx.h"

#include "compatibility.h"
#include "dialogs.h"
#include "resource.h"
#include "utils.h"
#include "externs.h"

#include <commdlg.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <Aygshell.h>

#include <windows.h>

#include <tchar.h>

void BuildLabelList();
void buildSwatchControlArray(HWND, HWND, int, int);
void InitAirspaceList(HWND hListView);
void InitImList(HIMAGELIST hImList);
void UpdateImListBMP(HIMAGELIST hImList, int item);


BOOL ChooseAirspaceStyle(HWND hWndParent, TCHAR* szText, int *iColour, int *iPattern);
HBITMAP CreateAirspaceSwatch(int cx, int cy, int iColour, int iPattern);

LRESULT CALLBACK AirspaceColourSelectDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AirspaceColourDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


typedef struct {
  long ID;
  TCHAR *szCaption;
} airspaceLabel;

typedef struct {
  BYTE iColour;
  BYTE iPattern;
  TCHAR text[256];
} airspaceStyle;


extern COLORREF Colours[];
extern HBRUSH hAirspaceBrushes[NUMAIRSPACEBRUSHES];
extern HBITMAP hAirspaceBitmap[NUMAIRSPACEBRUSHES];
extern TCHAR szRegistryAirspaceBlackOutline[];


static const int swatchWidth  = 24;
static const int swatchHeight = 24;
static const int swatchSpacing = 4;
static const int swatchOuterBorder = 14;

static const int colourSwatchIDBase  = 50;
static const int patternSwatchIDBase = colourSwatchIDBase + NUMAIRSPACECOLORS;

static const int numAirspaceTypes = 12;


static airspaceLabel labels[numAirspaceTypes] = {
  { CLASSA,     _T("Class A")},
  { CLASSB,     _T("Class B")},
  { CLASSC,     _T("Class C")},
  { CLASSD,     _T("Class D")},
  { PROHIBITED, _T("Prohibited areas")},
  { DANGER,     _T("Danger areas")},
  { RESTRICT,   _T("Restricted areas")},
  { CTR,        _T("CTR")},
  { NOGLIDER,   _T("No Gliders")},
  { WAVE,       _T("Wave")},
  { OTHER,      _T("Other")},
  { AATASK,     _T("AAT")}
};


LRESULT CALLBACK AirspaceColourDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  
  static HWND       hListView = NULL;
  static HIMAGELIST hImList   = NULL;
  static HFONT      hFont     = NULL;
  
  LOGFONT logfont;
  LPNMHDR nmhdr = NULL;
  
  DWORD dwExStyle;
  
  switch (message)
  {
  case WM_INITDIALOG:
    
    // Initialize airspace outline checkbox
    if(bAirspaceBlackOutline) {
      SendDlgItemMessage(hDlg,IDC_BLACKOUTLINE,BM_SETCHECK,BST_CHECKED,0);
    } else {
      SendDlgItemMessage(hDlg,IDC_BLACKOUTLINE,BM_SETCHECK,BST_UNCHECKED,0);
    }
    
    // Get handle to the ListView control
    hListView = GetDlgItem(hDlg,IDC_LISTAIRSPACE);
    
    // Use a slightly larger font than normal in the listview
    memset ((char *)&logfont, 0, sizeof (logfont));  
    logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
    logfont.lfHeight = (int) (0.8*swatchHeight);
#ifndef NOCLEARTYPE
    logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY;
#endif
    
    hFont = CreateFontIndirect (&logfont);

    // Apply font to window
    SendMessage(hListView,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));


    // Create an image list
    hImList = ImageList_Create(swatchWidth, swatchHeight, ILC_COLOR, numAirspaceTypes, 0);
    
    
    dwExStyle = ListView_GetExtendedListViewStyle(hListView);
    dwExStyle |= LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT;
    ListView_SetExtendedListViewStyle(hListView, dwExStyle);

    // Populate image list with images
    InitImList(hImList);
    
    // Associate ImageList with the ListView control
    ListView_SetImageList(hListView, hImList, LVSIL_SMALL);
    
    // Populate ListView control with items
    InitAirspaceList(hListView);

    // There's a funny bug that stops the vertical
    // scroll bar being displayed (usually on the
    // second opening of the settings window).
    // This is a fudge to make sure that it appears
    ListView_EnsureVisible(hListView, numAirspaceTypes-1, FALSE);
    ListView_EnsureVisible(hListView,  0, FALSE);
    
    SetFocus(hListView);

    return TRUE;
    
  case WM_NOTIFY:
    nmhdr = (LPNMHDR) lParam;
    
    switch (nmhdr->idFrom) {
    case IDC_LISTAIRSPACE:
      
      if (nmhdr->code == LVN_ITEMACTIVATE) {

        int item = ((LPNMLISTVIEW) lParam)->iItem;
        int airID = labels[item].ID;
        
        // Allow user to choose different style
        if (ChooseAirspaceStyle(hDlg, labels[item].szCaption,
            &iAirspaceColour[airID], &iAirspaceBrush[airID]))
        {
          
          // Update listbox icons
          UpdateImListBMP(hImList, item);
          ListView_Update(hListView, item);
          UpdateWindow(hListView);
          
          // Write changes to registry
          SetRegistryColour(airID,iAirspaceColour[airID]);
          SetRegistryBrush (airID, iAirspaceBrush[airID]);
        }
        
        // Unselect the item
        ListView_SetItemState(hListView, item, 0, LVIS_SELECTED);
        SetFocus(hDlg);

        return 0;
      }
      break;
    }
    
    break;
    
    case WM_COMMAND:

      switch (LOWORD(wParam)) {
      case IDC_BLACKOUTLINE:
        bAirspaceBlackOutline = !bAirspaceBlackOutline;
        SetToRegistry(szRegistryAirspaceBlackOutline,bAirspaceBlackOutline);
        return TRUE;
      }

      break;

    case WM_DESTROY:      
      hListView = NULL;
      if (hFont) {
        DeleteObject(hFont);
        hFont = NULL;
      }
      
      return 0;
  }
  
  return FALSE;
}


/*****************************************
initAirspaceList

  Populates the ListView control with
  Airspace colour entries
  
*****************************************/
void InitAirspaceList(HWND hListView)
{
  int i;
  LVITEM LvItem;
  LVCOLUMN LvCol;
    
  // Set column attribs
  memset(&LvCol,0,sizeof(LvCol));                  // Zero Members
  LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;    // Type of mask
  LvCol.cx=0x28;                                   // width of coloum
  LvCol.pszText=_T("Airspace type");               // First Header Text
  
  
  // Insert column heading
  SendMessage(hListView,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
  
  
  // Add the airspace entries
  memset(&LvItem,0,sizeof(LvItem));
  LvItem.mask=LVIF_TEXT | LVIF_IMAGE;
  LvItem.cchTextMax = 64;  // Size of text buffer
  LvItem.iSubItem=0;       // Put in first column
  
  for (i=0; i<numAirspaceTypes; ++i) {
    LvItem.iItem=i;
    LvItem.iImage=i;
    LvItem.pszText=labels[i].szCaption;
    SendMessage(hListView,LVM_INSERTITEM,0,(LPARAM)&LvItem); // Send info to the Listview
  }
}

/*********************************************************
initImageList

  Fills image list with a group of swatches representing
  the current airspace colour selections
*********************************************************/
void InitImList(HIMAGELIST hImList)
{
  int i, airID;
  HBITMAP hBMP;
  
  for (i=0; i<numAirspaceTypes; ++i) {
    
    airID = labels[i].ID;

    // Create swatch
    hBMP = CreateAirspaceSwatch(swatchWidth, swatchHeight, iAirspaceColour[airID], iAirspaceBrush[airID]);
    
    // Whack it in the image list
    ImageList_Add(hImList, hBMP, NULL);
    
    // Get rid of the bmp
    DeleteObject(hBMP);
    
  }
}

/*********************************************************
UpdateImListBMP

  Replace one image in the image list
 *********************************************************/
void UpdateImListBMP(HIMAGELIST hImList, int item)
{
  HBITMAP hBMP;
  
  int airID = labels[item].ID;

  hBMP = CreateAirspaceSwatch(swatchWidth, swatchHeight,
    iAirspaceColour[airID], iAirspaceBrush[airID]);

  ImageList_Replace(hImList, item, hBMP, NULL);

  DeleteObject(hBMP);
}

/*********************************************************
CreateAirspaceSwatch

  Caller must use DeleteObject on the returned BMP handle,
  once they've finished with it
*********************************************************/
HBITMAP CreateAirspaceSwatch(int cx, int cy, int iColour, int iPattern)
{
  HBITMAP hBMP;
  HDC hdc, hdcMem;
  
  // A slightly messed up way of creating a bitmap compatible
  // with the device's display
  hdc = GetDC(hWndMainWindow);
  hdcMem = CreateCompatibleDC(hdc);
  ReleaseDC(hWndMainWindow, hdc);
  
  hBMP   = CreateBitmap(cx, cy, 1, GetDeviceCaps(hdcMem, BITSPIXEL), NULL);
  
  // Paint airspace swatch into bitmap
  SelectObject(hdcMem, hBMP);
  SetTextColor(hdcMem, Colours[iColour]);
  SetBkColor(hdcMem, RGB(0xFF, 0xFF, 0xFF));
  SelectObject(hdcMem, hAirspaceBrushes[iPattern]);
  Rectangle(hdcMem, 0, 0, cx, cy);
  
  DeleteDC(hdcMem);
  
  return hBMP;
}


/*********************************************************
ChooseAirspaceStyle

  Present the user with dialog box from which they can
  choose a different airspace style
  
  Set iColour and iPattern to initial values
  If the user makes changes, they will be returned
  in iColour and iPattern
    
  Returns TRUE if OK button was pressed (change made)
  FALSE if Cancel was pressed (no change made)
*********************************************************/

BOOL ChooseAirspaceStyle(HWND hWndParent, TCHAR* szText, int *iColour, int *iPattern)
{
  int retval;
  
  airspaceStyle vals;
  
  vals.iColour  = *iColour;
  vals.iPattern = *iPattern;
  _tcscpy(vals.text, szText);

  
  retval = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_AIRSPACECOLOURSEL), hWndParent,
    (DLGPROC) AirspaceColourSelectDlg, (LPARAM) &vals);
  
  if (retval > 0) {
    *iColour  = vals.iColour;
    *iPattern = vals.iPattern;
    return TRUE;
  }
  
  return FALSE;
  
}


// This is the window proc for the modal dialog box, containing
// all the coloured swatches to choose from
LRESULT CALLBACK AirspaceColourSelectDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

  static airspaceStyle* outputVals;
  static airspaceStyle  vals;
  
  static HFONT hFont;
  
  RECT rc;
  HWND   hBtnOld, hBtnNew;
  HBRUSH hBrush;
  LOGFONT logfont;
  LPDRAWITEMSTRUCT pdis;

  int item;
  long wID;

  switch (message) {

  case WM_INITDIALOG:
    // Get initial values in the form of a structure pointer
    // from the calling function
    // We'll write the results back out to this at the end
    outputVals = (airspaceStyle*) lParam;
    memcpy(&vals, outputVals, sizeof(vals)); // local copy

    // Set title text
    memset ((char *)&logfont, 0, sizeof (logfont));  
    logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
    logfont.lfWeight = FW_BOLD;
#ifndef NOCLEARTYPE
    logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY;
#endif
    
    hFont = CreateFontIndirect (&logfont);

    // Apply font to window
    SendMessage(GetDlgItem(hDlg,IDC_AIRSPACE_CLASS),WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

    // Change text
    SendMessage(GetDlgItem(hDlg,IDC_AIRSPACE_CLASS),WM_SETTEXT,0, (LPARAM) vals.text);

    // Build arrays of swatch controls
    buildSwatchControlArray(hDlg, GetDlgItem(hDlg, IDC_COLOUR_BOX),
                            NUMAIRSPACECOLORS, colourSwatchIDBase);
    buildSwatchControlArray(hDlg, GetDlgItem(hDlg, IDC_PATTERN_BOX),
                            NUMAIRSPACEBRUSHES, patternSwatchIDBase);    
    

    return TRUE;
    break;
    


  case WM_COMMAND:    
    
    switch (LOWORD(wParam)) {
    
    case IDC_OK:
      // Copy current values back out to the structure that
      // was given to us at the beginning through lParam
      outputVals->iColour  = vals.iColour;
      outputVals->iPattern = vals.iPattern;
      EndDialog(hDlg, 1);
      break;
    
    case IDC_CANCEL:
      // Don't update the output vals
      EndDialog(hDlg, 0);
      break;
    }

    wID = LOWORD(wParam);
    if (wID >= colourSwatchIDBase &&
        wID <= colourSwatchIDBase + NUMAIRSPACECOLORS - 1) {
      // Colour swatch was pressed

      // Get handles of controls representing
      // old and new colours
      hBtnOld = GetDlgItem(hDlg, colourSwatchIDBase + vals.iColour);
      hBtnNew = GetDlgItem(hDlg, wID);

      // set new colour value
      vals.iColour = wID - colourSwatchIDBase;

      // Update graphics of the two buttons
      InvalidateRect(hBtnOld, NULL, false);
      InvalidateRect(hBtnNew, NULL, false);
      UpdateWindow(hBtnOld);
      if (hBtnNew!=hBtnOld)
        UpdateWindow(hBtnNew);
      

      return TRUE;

    } else if (wID >= patternSwatchIDBase &&
               wID <= patternSwatchIDBase + NUMAIRSPACEBRUSHES - 1) {
      // Pattern swatch was pressed

      // Get handles of controls representing
      // old and new patterns
      hBtnOld = GetDlgItem(hDlg, patternSwatchIDBase + vals.iPattern);
      hBtnNew = GetDlgItem(hDlg, wID);

      // set new pattern value
      vals.iPattern = wID - patternSwatchIDBase;

      // Update graphics of the two buttons
      InvalidateRect(hBtnOld, NULL, false);
      InvalidateRect(hBtnNew, NULL, false);
      UpdateWindow(hBtnOld);
      if (hBtnNew!=hBtnOld)
        UpdateWindow(hBtnNew);

      return TRUE;
    }

    break;    
    

    case WM_DRAWITEM :
      pdis = (LPDRAWITEMSTRUCT) lParam;

      if (pdis->CtlID >= colourSwatchIDBase &&
          pdis->CtlID <= colourSwatchIDBase+NUMAIRSPACECOLORS-1) {

        // This is the part where we draw a colour swatch control
        item = pdis->CtlID - colourSwatchIDBase;
        
        hBrush = CreateSolidBrush(Colours[item]);
        FillRect(pdis->hDC, &pdis->rcItem, hBrush);
        DeleteObject(hBrush);

        SelectObject(pdis->hDC, GetStockObject(HOLLOW_BRUSH));
        Rectangle(pdis->hDC, 0, 0, pdis->rcItem.right, pdis->rcItem.bottom);


        if (pdis->itemState & ODS_SELECTED) {
          DrawEdge(pdis->hDC, &pdis->rcItem, EDGE_SUNKEN, BF_RECT);
        } else if (vals.iColour == item) {
          // This control represents the currently selected
          // item, so draw a raised border round it
          rc = pdis->rcItem;
          InflateRect(&rc, -1, -1); // Don't cover border rectangle
          
          DrawEdge(pdis->hDC, &rc, EDGE_RAISED, BF_RECT);

        }

        return 0;
      } else if (pdis->CtlID >= patternSwatchIDBase &&
                 pdis->CtlID <= patternSwatchIDBase+NUMAIRSPACEBRUSHES-1) {

        // This is the part where we draw a pattern swatch control
        item = pdis->CtlID - patternSwatchIDBase;
        
        FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH) GetStockObject(WHITE_BRUSH));
        FillRect(pdis->hDC, &pdis->rcItem, hAirspaceBrushes[item]);

        SelectObject(pdis->hDC, GetStockObject(HOLLOW_BRUSH));        
        Rectangle(pdis->hDC, 0, 0, pdis->rcItem.right, pdis->rcItem.bottom);


        if (pdis->itemState & ODS_SELECTED) {
          DrawEdge(pdis->hDC, &pdis->rcItem, EDGE_SUNKEN, BF_RECT);
        } else if (vals.iPattern == item) {
          // This control represents the currently selected
          // colour, so draw a raised border round it
          rc = pdis->rcItem;
          InflateRect(&rc, -1, -1); // Don't cover border rectangle
          
          DrawEdge(pdis->hDC, &rc, EDGE_RAISED, BF_RECT);

        }

        return 0;
      }
      break;
    case WM_DESTROY:
      DeleteObject(hFont);

      return TRUE;

  }
  
  return FALSE;
  
}

/*************************************************************
buildSwatchControlArray

  Fill dialog with swatches, positioned within
  a given container control
  This is called for both colour swatches and pattern swatches
 *************************************************************/
void buildSwatchControlArray(HWND hDlg, HWND hWndContainer,
                             int numSwatches, int IDBase)
{
  int i;
  int cols;
  int x, y, px, py;
  
  RECT rc;
  HWND hButton = NULL;
  
 
  // First work out how many columns we can squeeze in
  // to the colour box
  GetClientRect(hWndContainer, &rc);
  MapWindowPoints(hWndContainer, hDlg, (LPPOINT) &rc, 2);
  cols = 1 + (rc.right - rc.left - swatchWidth - (2*swatchOuterBorder)) / (swatchWidth + swatchSpacing);
  
  // Now Create the controls  
  for (i=0, x=0, y=0; i<numSwatches; ++i) {
        
    // We're using ownerdraw buttons
    hButton = CreateWindow(_T("button"), _T(""),
                  WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                  0, 0, 0, 0,
                  hDlg, (HMENU)(IDBase+i), hInst, NULL);

    // We want to position the swatches within the 'Colour' frame control
    px = rc.left + swatchOuterBorder + x*(swatchWidth +swatchSpacing);
    py = rc.top  + swatchOuterBorder + y*(swatchHeight+swatchSpacing);
    
    SetWindowPos(hButton, HWND_TOP, px, py, swatchWidth, swatchHeight, 0);
        
    if ((++x) >= cols) {
      y++; x=0;
    }
    
  }
}
