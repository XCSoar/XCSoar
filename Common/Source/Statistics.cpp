#include "Statistics.h"
#include "XCSoar.h"
#include "Externs.h"
#include "McReady.h"
#include "Units.h"

extern HFONT                                   StatisticsFont;


double Statistics::yscale;
double Statistics::xscale;
double Statistics::y_min;
double Statistics::x_min;
double Statistics::x_max;
double Statistics::y_max;
bool Statistics::unscaled_x;
bool Statistics::unscaled_y;



void Statistics::ResetScale() {
  unscaled_y = true;  
  unscaled_x = true;  
}


void Statistics::ScaleYFromData(RECT rc, LeastSquares* lsdata) 
{
  if (!lsdata->sum_n) {
    return;
  }

  if (unscaled_y) {
    y_min = lsdata->y_min;
    y_max = lsdata->y_max;
    unscaled_y = false;
  } else {
    y_min = min(y_min,lsdata->y_min);
    y_max = max(y_max,lsdata->y_max);
  }

  if (lsdata->sum_n>1) {
    double y0, y1;
    y0 = lsdata->x_min*lsdata->m+lsdata->b;
    y1 = lsdata->x_max*lsdata->m+lsdata->b;
    y_min = min(y_min,min(y0,y1));
    y_max = max(y_max,max(y0,y1));
  }


  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top)/yscale;
  }
}


void Statistics::ScaleXFromData(RECT rc, LeastSquares* lsdata) 
{
  if (!lsdata->sum_n) {
    return;
  }
  if (unscaled_x) {
    x_min = lsdata->x_min;
    x_max = lsdata->x_max;
    unscaled_x = false;
  } else {
    x_min = min(x_min,lsdata->x_min);
    x_max = max(x_max,lsdata->x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left)/xscale;
  }
}


void Statistics::ScaleYFromValue(RECT rc, double value) 
{
  if (unscaled_y) {
    y_min = value;
    y_max = value;
    unscaled_y = false;
  } else {
    y_min = min(value, y_min);
    y_max = max(value, y_max);
  }

  yscale = (y_max - y_min);
  if (yscale>0) {
    yscale = (rc.bottom-rc.top)/yscale;
  }
}


void Statistics::ScaleXFromValue(RECT rc, double value) 
{
  if (unscaled_x) {
    x_min = value;
    x_max = value;
    unscaled_x = false;
  } else {
    x_min = min(value, x_min);
    x_max = max(value, x_max);
  }

  xscale = (x_max-x_min);
  if (xscale>0) {
    xscale = (rc.right-rc.left)/xscale;
  }

}


void Statistics::StyleLine(HDC hdc, POINT l1, POINT l2,
                           int Style) {
  POINT line[2];
  line[0] = l1;
  line[1] = l2;
  switch (Style) {
  case STYLE_BLUETHIN:
    DrawDashLine(hdc, 1, 
                 l1, 
                 l2, 
                 RGB(0,0,255));
    break;
  case STYLE_REDTHICK:
    DrawDashLine(hdc, 3, 
                 l1, 
                 l2, 
                 RGB(200,50,50));
    break;
  case STYLE_DASHGREEN:
    DrawDashLine(hdc, 2, 
                 line[0], 
                 line[1], 
                 RGB(0,255,0));
    break;
  case STYLE_MEDIUMBLACK:
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Polyline(hdc, line, 2);
    break;
  case STYLE_THINDASHPAPER:
    DrawDashLine(hdc, 1, 
                 l1, 
                 l2, 
                 RGB(0xf0,0xf0,0xb0));    
    break;

  default:
    break;
  }

}


void Statistics::DrawTrend(HDC hdc, RECT rc, LeastSquares* lsdata, int Style) 
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = lsdata->x_min;
  xmax = lsdata->x_max;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;
  
  xmin = (int)((xmin-x_min)*xscale)+rc.left;
  xmax = (int)((xmax-x_min)*xscale)+rc.left;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawTrendN(HDC hdc, RECT rc, LeastSquares* lsdata, 
                            int Style) 
{
  if (lsdata->sum_n<2) {
    return;
  }

  if (unscaled_x || unscaled_y) {
    return;
  }

  double xmin, xmax, ymin, ymax;
  xmin = 0.5;
  xmax = lsdata->sum_n+0.5;
  ymin = lsdata->x_min*lsdata->m+lsdata->b;
  ymax = lsdata->x_max*lsdata->m+lsdata->b;
  
  xmin = (int)((xmin)*xscale)+rc.left;
  xmax = (int)((xmax)*xscale)+rc.left;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawLine(HDC hdc, RECT rc, double xmin, double ymin,
                          double xmax, double ymax,
                          int Style) {

  if (unscaled_x || unscaled_y) {
    return;
  }
  
  xmin = (int)((xmin-x_min)*xscale)+rc.left;
  xmax = (int)((xmax-x_min)*xscale)+rc.left;
  ymin = (int)((y_max-ymin)*yscale)+rc.top;
  ymax = (int)((y_max-ymax)*yscale)+rc.top;
  POINT line[2];
  line[0].x = (int)xmin;
  line[0].y = (int)ymin;
  line[1].x = (int)xmax;
  line[1].y = (int)ymax;

  // STYLE_REDTHICK
  StyleLine(hdc, line[0], line[1], Style);

}


void Statistics::DrawBarChart(HDC hdc, RECT rc, LeastSquares* lsdata) {
  int i;

  if (unscaled_x || unscaled_y) {
    return;
  }

  SelectObject(hdc, GetStockObject(BLACK_PEN));
  SelectObject(hdc, GetStockObject(BLACK_BRUSH));

  int xmin, ymin, xmax, ymax;

  for (i=0; i<lsdata->sum_n; i++) {
    xmin = (int)((i+0.2)*xscale)+rc.left;
    ymin = (int)((y_max-y_min)*yscale)+rc.top;
    xmax = (int)((i+0.8)*xscale)+rc.left;
    ymax = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    Rectangle(hdc, 
              xmin, 
              ymin,
              xmax,
              ymax);
  }

}


void Statistics::DrawLineGraph(HDC hdc, RECT rc, LeastSquares* lsdata,
                               int Style) {

  POINT line[2];

  int i;


  int xmin, ymin, xmax, ymax;

  for (i=0; i<lsdata->sum_n-1; i++) {
    xmin = (int)((lsdata->xstore[i]-x_min)*xscale)+rc.left;
    ymin = (int)((y_max-lsdata->ystore[i])*yscale)+rc.top;
    xmax = (int)((lsdata->xstore[i+1]-x_min)*xscale)+rc.left;
    ymax = (int)((y_max-lsdata->ystore[i+1])*yscale)+rc.top;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_DASHGREEN
    // STYLE_MEDIUMBLACK
    StyleLine(hdc, line[0], line[1], Style);
  }
}


void Statistics::DrawXGrid(HDC hdc, RECT rc, double ticstep, double zero,
                           int Style) {

  POINT line[2];

  double xval;

  int xmin, ymin, xmax, ymax;

  for (xval=zero; xval<= x_max; xval+= ticstep) {

    xmin = (int)((xval-x_min)*xscale)+rc.left;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

  for (xval=zero; xval>= x_min; xval-= ticstep) {

    xmin = (int)((xval-x_min)*xscale)+rc.left;
    ymin = rc.top;
    xmax = xmin;
    ymax = rc.bottom;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

}

void Statistics::DrawYGrid(HDC hdc, RECT rc, double ticstep, double zero,
                           int Style) {

  POINT line[2];

  double yval;

  int xmin, ymin, xmax, ymax;

  for (yval=zero; yval<= y_max; yval+= ticstep) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }

  for (yval=zero; yval>= y_min; yval-= ticstep) {

    xmin = rc.left;
    ymin = (int)((y_max-yval)*yscale)+rc.top;
    xmax = rc.right;
    ymax = ymin;
    line[0].x = xmin;
    line[0].y = ymin;
    line[1].x = xmax;
    line[1].y = ymax;

    // STYLE_THINDASHPAPER
    StyleLine(hdc, line[0], line[1], Style);
  }
}




/////////////////


void Statistics::RenderBarograph(HDC hdc, RECT rc)
{

  ResetScale();
  ScaleXFromData(rc, &flightstats.Altitude);
  ScaleYFromData(rc, &flightstats.Altitude);
  ScaleXFromData(rc, &flightstats.Altitude_Base);
  ScaleYFromData(rc, &flightstats.Altitude_Base);
  ScaleXFromData(rc, &flightstats.Altitude_Ceiling);
  ScaleYFromData(rc, &flightstats.Altitude_Ceiling);

  DrawXGrid(hdc, rc, 
            0.25, flightstats.Altitude.x_min,
            STYLE_THINDASHPAPER);

  DrawYGrid(hdc, rc, 1000/ALTITUDEMODIFY, 0, STYLE_THINDASHPAPER);

  DrawLineGraph(hdc, rc, &flightstats.Altitude,
                STYLE_MEDIUMBLACK);

  DrawTrend(hdc, rc, &flightstats.Altitude_Base, STYLE_BLUETHIN);

  DrawTrend(hdc, rc, &flightstats.Altitude_Ceiling, STYLE_BLUETHIN);

}


void Statistics::RenderClimb(HDC hdc, RECT rc) 
{
  ResetScale();
  ScaleYFromData(rc, &flightstats.ThermalAverage);
  ScaleYFromValue(rc, MCCREADY/LIFTMODIFY);
  ScaleYFromValue(rc, 0);
  ScaleXFromValue(rc, 0);
  ScaleXFromValue(rc, flightstats.ThermalAverage.sum_n+1);
  
  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER);
  
  DrawBarChart(hdc, rc,
               &flightstats.ThermalAverage);
  DrawLine(hdc, rc,
           0, MCCREADY/LIFTMODIFY, 
           flightstats.ThermalAverage.sum_n+1,
           MCCREADY/LIFTMODIFY,
           STYLE_REDTHICK);
  
  DrawTrendN(hdc, rc,
             &flightstats.ThermalAverage,
             STYLE_BLUETHIN);
}


void Statistics::RenderGlidePolar(HDC hdc, RECT rc)
{

  ResetScale();
  ScaleYFromValue(rc, 0);
  ScaleYFromValue(rc, GlidePolar::SinkRateFast(0,(int)(SAFTEYSPEED-1)));
  ScaleXFromValue(rc, 0); // GlidePolar::Vminsink);
  ScaleXFromValue(rc, SAFTEYSPEED);
  
  DrawXGrid(hdc, rc, 
            10.0/SPEEDMODIFY, 0,
            STYLE_THINDASHPAPER);
  DrawYGrid(hdc, rc, 
            1.0/LIFTMODIFY, 0,
            STYLE_THINDASHPAPER);
  
  int i;
  double sinkrate0, sinkrate1;
  for (i= GlidePolar::Vminsink; i< SAFTEYSPEED-1;
       i++) {
    
    sinkrate0 = GlidePolar::SinkRateFast(0,i);
    sinkrate1 = GlidePolar::SinkRateFast(0,i+1);
    DrawLine(hdc, rc,
             i, sinkrate0 , 
             i+1, sinkrate1, 
             STYLE_MEDIUMBLACK);
  }
}



////////////////

LRESULT CALLBACK AnalysisProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  TCHAR Temp[2048];
  static HDC hdcScreen;
  static int page = 0;
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rc;
  static RECT rcgfx;
  HFONT hfOld;
  HBRUSH background;

  switch (message)
    {
    case WM_INITDIALOG:
                 
      hdcScreen = GetDC(hDlg);
      GetClientRect(hDlg, &rc);

      SendDlgItemMessage(hDlg, IDC_ANALYSISLABEL, WM_SETFONT,
                  (WPARAM)StatisticsFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg, IDC_ANALYSISTEXT, WM_SETFONT,
                  (WPARAM)StatisticsFont,MAKELPARAM(TRUE,0));

      rcgfx = rc;
      rcgfx.left  += 10;
      rcgfx.right -= 10;
      rcgfx.top = (rc.bottom-rc.top)*2/10+rc.top;
      rcgfx.bottom = (rc.bottom-rc.top)*2/3+rc.top;
      
      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) 
        {
          ::ReleaseDC(hDlg, hdcScreen);
          EndDialog(hDlg, LOWORD(wParam));
          MapWindow::RequestFastRefresh= true;
          ClearAirspaceWarnings(false); // airspace warning gets refreshed
          FullScreen();
          return TRUE;
        }
      if (LOWORD(wParam) == IDC_ANALYSISNEXT) {
        page++;
        
        // cycle around to start page
        if (page==3) {
          page=0;
        }
      }
    case WM_PAINT:

      // make background white
      GetClientRect(hDlg, &rc);
      hdc = BeginPaint(hDlg, &ps);

      background = CreateSolidBrush(RGB(0xf0,0xf0,0xb0));
      HGDIOBJ gTemp;
      gTemp = SelectObject(hdcScreen, background);
      SelectObject(hdcScreen, GetStockObject(WHITE_PEN));

      Rectangle(hdcScreen,rcgfx.left,rcgfx.top,rcgfx.right,rcgfx.bottom);
      DeleteObject(background);

      hfOld = (HFONT)SelectObject(hdcScreen, StatisticsFont);
      
      if (page==0) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, TEXT("Barograph"));

        wsprintf(Temp, TEXT("Working band: %5.0f-%5.0f %s\r\nCeiling trend: %5.0f %s/hr"),
                 flightstats.Altitude_Base.y_ave*ALTITUDEMODIFY, 
                 flightstats.Altitude_Ceiling.y_ave*ALTITUDEMODIFY, 
                 Units::GetAltitudeName(),
                 flightstats.Altitude_Ceiling.m*ALTITUDEMODIFY,
                 Units::GetAltitudeName());

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderBarograph(hdcScreen, rcgfx);


      }
      if (page==1) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, TEXT("Climb"));

        wsprintf(Temp, TEXT("Average climb rate: %3.1f %s\r\nClimb trend: %3.2f %s"),                 
                  flightstats.ThermalAverage.y_ave*LIFTMODIFY,
                 Units::GetVerticalSpeedName(),
                 flightstats.ThermalAverage.m*LIFTMODIFY,
                 Units::GetVerticalSpeedName()
                 );

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderClimb(hdcScreen, rcgfx);

      }
      if (page==2) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, TEXT("Glide Polar"));

        wsprintf(Temp, TEXT("Best LD: %3.1f at %3.0f %s\r\nMin sink: %3.2f %s at %3.0f %s"),
                 GlidePolar::bestld,
                 GlidePolar.Vbestld*SPEEDMODIFY,
                 Units::GetHorizontalSpeedName(),
                 GlidePolar::minsink*LIFTMODIFY,
                 Units::GetVerticalSpeedName(),
                 GlidePolar::Vminsink*SPEEDMODIFY,
                 Units::GetHorizontalSpeedName());

        SetDlgItemText(hDlg,IDC_ANALYSISTEXT, Temp);

        Statistics::RenderGlidePolar(hdcScreen, rcgfx);

      }
      if (page==3) {
        SetDlgItemText(hDlg,IDC_ANALYSISLABEL, TEXT("Something"));
      }

      SelectObject(hdcScreen, hfOld);
      EndPaint(hDlg, &ps);

      return FALSE;

    case WM_CLOSE:
      MapWindow::RequestFastRefresh= true;
      ClearAirspaceWarnings(false); // airspace warning gets refreshed
      FullScreen();
    }
  return FALSE;
}
