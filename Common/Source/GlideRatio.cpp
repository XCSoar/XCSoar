#include "GlideRatio.hpp"
#include "XCSoar.h"
#include "externs.h"

#include <string.h>

void InitLDRotary(ldrotary_s *buf)
{
short i, bsize;

	switch (AverEffTime) {
		case ae15seconds:
			bsize=15;	// useless, LDinst already there
			break;
		case ae30seconds:
			bsize=30;	// limited useful
			break;
		case ae60seconds:
			bsize=60;	// starting to be valuable
			break;
		case ae90seconds:
			bsize=90;	// good interval
			break;
		case ae2minutes:
			bsize=120;	// other software's interval
			break;
		case ae3minutes:
			bsize=180;	// probably too long interval
			break;
		default:
			bsize=3; // make it evident
			break;
	}
	//if (bsize <3 || bsize>MAXLDROTARYSIZE) return false;
	for (i=0; i<MAXLDROTARYSIZE; i++) {
		buf->distance[i]=0;
		buf->altitude[i]=0;
	}
	buf->totaldistance=0;
	buf->start=-1;
	buf->size=bsize;
	buf->valid=false;
}

void InsertLDRotary(ldrotary_s *buf, int distance, int altitude) {
static short errs=0;

	if (CALCULATED_INFO.OnGround == TRUE) {
		return;
	}
	if (CALCULATED_INFO.Circling == TRUE) {
		return;
	}

	if (distance<3 || distance>150) { // just ignore, no need to reset rotary
		if (errs>2) {
			InitLDRotary(&rotaryLD);
			errs=0;
			return;

		}
		errs++;
		return;
	}
	errs=0;
	if (++buf->start >=buf->size) {
		buf->start=0;
		buf->valid=true; // flag for a full usable buffer
	}
	// need to fill up buffer before starting to empty it
	if ( buf->valid == true) buf->totaldistance-=buf->distance[buf->start];
	buf->totaldistance+=distance;
	buf->distance[buf->start]=distance;
	buf->altitude[buf->start]=altitude;
}

/*
 * returns 0 if invalid, 999 if too high

 */
int CalculateLDRotary(ldrotary_s *buf ) {

	int altdiff, eff;
	short bcold;

	if ( CALCULATED_INFO.Circling == TRUE || CALCULATED_INFO.OnGround == TRUE) {
		return(0);
	}

	if ( buf->start <0) {

		return(0);
	}

	ldrotary_s bc;
  	memcpy(&bc, buf, sizeof(ldrotary_s));

	if (bc.valid == false ) {
		if (bc.start==0) return(0); // unavailable
		bcold=0;
	} else {

		if (bc.start < (bc.size-1))
			bcold=bc.start+1;
		else
			bcold=0;
	}

	altdiff= bc.altitude[bcold] - bc.altitude[bc.start];
	if (altdiff == 0 ) return(INVALID_GR); // infinitum
	eff= bc.totaldistance / altdiff;

	if (eff>MAXEFFICIENCYSHOW) eff=INVALID_GR;

	return(eff);

}
