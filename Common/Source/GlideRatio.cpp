#include "GlideRatio.hpp"
#include "XCSoar.h"
#include "externs.h"

#include <string.h>

bool InitLDRotary(ldrotary_s *buf) {
short i, bsize;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif

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
#ifdef DEBUG_ROTARY
	// John, DEBUGROTARY is used only on PC, no need to make it any better, it will be removed
	sprintf(ventabuffer,"InitLdRotary size=%d\r\n",buf->size);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
  return false; //RLD Paolo, I added this so it would compile correctly.  Not sure what the correct value is.
                //VNT Rob, it's an unused bool type so any value is ok, Ill make it void eventually
}

void InsertLDRotary(ldrotary_s *buf, int distance, int altitude) {
static short errs=0;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif

	if (CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"OnGround, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	if (CALCULATED_INFO.Circling == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Circling, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}

	if (distance<3 || distance>150) { // just ignore, no need to reset rotary
		if (errs>2) {
#ifdef DEBUG_ROTARY
			sprintf(ventabuffer,"Rotary reset after exceeding errors\r\n");
			if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
				    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
			InitLDRotary(&rotaryLD);
			errs=0;
			return;

		}
		errs++;
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"(errs=%d) IGNORE INVALID distance=%d altitude=%d\r\n",errs,distance,altitude);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	errs=0;
	if (++buf->start >=buf->size) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"*** rotary reset and VALID=TRUE ++bufstart=%d >=bufsize=%d\r\n",buf->start, buf->size);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		buf->start=0;
		buf->valid=true; // flag for a full usable buffer
	}
	// need to fill up buffer before starting to empty it
	if ( buf->valid == true) buf->totaldistance-=buf->distance[buf->start];
	buf->totaldistance+=distance;
	buf->distance[buf->start]=distance;
	buf->altitude[buf->start]=altitude;
#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"insert buf[%d/%d], distance=%d totdist=%d\r\n",buf->start, buf->size-1, distance,buf->totaldistance);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
		    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
}

/*
 * returns 0 if invalid, 999 if too high

 */
int CalculateLDRotary(ldrotary_s *buf ) {

	int altdiff, eff;
	short bcold;
#ifdef DEBUG_ROTARY
	char ventabuffer[200];
	FILE *fp;
#endif

	if ( CALCULATED_INFO.Circling == TRUE || CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Not Calculating, on ground or circling\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return(0);
	}

	if ( buf->start <0) {

#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Calculate: invalid buf start<0\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
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

#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"bcstart=%d bcold=%d altnew=%d altold=%d altdiff=%d totaldistance=%d eff=%d\r\n",
		bc.start, bcold,
		bc.altitude[bc.start], bc.altitude[bcold], altdiff, bc.totaldistance, eff);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif

	if (eff>MAXEFFICIENCYSHOW) eff=INVALID_GR;

	return(eff);

}
