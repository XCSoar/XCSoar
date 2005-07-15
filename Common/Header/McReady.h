#if !defined(AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MCREADY_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

double McReadyAltitude(double MCREADY, double Distance, double Bearing, double WindSpeed, double WindBearing,
                       double *BestCruiseTrack, double *VMcReady, int isFinalGlide, double *timetogo);

double SinkRate(double Vias);
double SinkRate(double Vias, double loadfactor);
void SetBallast();

#endif
