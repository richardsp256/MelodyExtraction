#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Modified from:
 * http://staffwww.dcs.shef.ac.uk/people/N.Ma/resources/gammatone/gammatone_c.c
 *=========================================================================
 * An efficient C implementation of the 4th order gammatone filter
 *-------------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *-------------------------------------------------------------------------
 *
 *  gammatoneFilter(float *x, float **bm, float cf, int fs, int nsamples) 
 *
 *  x        - input signal
 *  bm       - basilar membrane displacement
 *  fs       - sampling frequency (Hz)
 *  cf       - center frequency of the filter (Hz)
 *  nsamples - number of samples in x
 *
 *  The gammatone filter is commonly used in models of the auditory system.
 *  The algorithm is based on Martin Cooke's Ph.D work (Cooke, 1993) using 
 *  the base-band impulse invariant transformation. This implementation is 
 *  highly efficient in that a mathematical rearrangement is used to 
 *  significantly reduce the cost of computing complex exponentials. For 
 *  more detail on this implementation see
 *  http://www.dcs.shef.ac.uk/~ning/resources/gammatone/
 *
 *  Originally written by:
 *  Ning Ma, University of Sheffield
 *  n.ma@dcs.shef.ac.uk, 09 Mar 2006
 *
 *  This code has been updated to use single precision floating point numbers
 *
 *=========================================================================
 */


/*=======================
 * Useful Const
 *=======================
 */
#define BW_CORRECTION      1.0190
#define VERY_SMALL_NUMBER  1.e-35
#ifndef M_PI
#define M_PI               3.14159265358979323846
#endif

/*=======================
 * Utility functions
 *=======================
 */
#define myMax(x,y)     ( ( x ) > ( y ) ? ( x ) : ( y ) )
#define myMod(x,y)     ( ( x ) - ( y ) * floor ( ( x ) / ( y ) ) )
#define erb(x)         ( 24.7 * ( 4.37e-3 * ( x ) + 1.0 ) )


double ERB(double f)
{
  //ERB = 24.7(4.37*10^-3 * f + 1)
  return (0.107939 * f) + 27;
}

//g(t) = a t^(n-1) e^(-2pi b t) cos(2pi f t + phase)
//n = 4
//b = 1.019 * ERB
//ERB(only when n=4) = 24.7(0.00437 f + 1)
//phase, in our case, can be safely ignored and removed.
//so with the above...
//g(t) = a t^(3) e^(-2pi t (0.109989841 f + 27.513)) cos(2pi f t)
void simpleGammatone(float* data, float** output, float centralFreq, int samplerate, int datalen)
{
  for(int i = 0; i < datalen; i++){
    double t = i/(double)samplerate;
    double bandwidth = BW_CORRECTION * ERB(centralFreq);
    double amplitude = data[i]; //this is (most likely) not actually the amplitude!!!

    (*output)[i] = amplitude * pow(i, 3) * exp(-2 * M_PI * bandwidth * t) * cos(2 * M_PI * centralFreq * t);
  }
}

void gammatoneFilter(float *x, float **bm, float cf, int fs, int nsamples)
{
   int t;
   float a, tpt, tptbw, gain;
   float p0r, p1r, p2r, p3r, p4r, p0i, p1i, p2i, p3i, p4i;
   float a1, a2, a3, a4, a5, u0r, u0i; /*, u1r, u1i;*/
   float qcos, qsin, oldcs, coscf, sincf;

  /*=========================================
   * Initialising variables
   *=========================================
   */
   tpt = ( M_PI + M_PI ) / fs;
   tptbw = tpt * erb ( cf ) * BW_CORRECTION;
   a = exp ( -tptbw );

  /* based on integral of impulse response */
   gain = ( tptbw*tptbw*tptbw*tptbw ) / 3;

  /* Update filter coefficients */
   a1 = 4.0*a; a2 = -6.0*a*a; a3 = 4.0*a*a*a; a4 = -a*a*a*a; a5 = a*a;
   p0r = 0.0; p1r = 0.0; p2r = 0.0; p3r = 0.0; p4r = 0.0;
   p0i = 0.0; p1i = 0.0; p2i = 0.0; p3i = 0.0; p4i = 0.0;
 
  /*===========================================================
   * exp(a+i*b) = exp(a)*(cos(b)+i*sin(b))
   * q = exp(-i*tpt*cf*t) = cos(tpt*cf*t) + i*(-sin(tpt*cf*t))
   * qcos = cos(tpt*cf*t)
   * qsin = -sin(tpt*cf*t)
   *===========================================================
   */
   coscf = cos ( tpt * cf );
   sincf = sin ( tpt * cf );
   qcos = 1; qsin = 0;   /* t=0 & q = exp(-i*tpt*t*cf)*/
   for ( t=0; t<nsamples; t++ )
   {
     /* Filter part 1 & shift down to d.c. */
      p0r = qcos*x[t] + a1*p1r + a2*p2r + a3*p3r + a4*p4r;
      p0i = qsin*x[t] + a1*p1i + a2*p2i + a3*p3i + a4*p4i;

     /* Clip coefficients to stop them from becoming too close to zero */
      if (fabs(p0r) < VERY_SMALL_NUMBER)
        p0r = 0.0F;
      if (fabs(p0i) < VERY_SMALL_NUMBER)
        p0i = 0.0F;
      
     /* Filter part 2 */
      u0r = p0r + a1*p1r + a5*p2r;
      u0i = p0i + a1*p1i + a5*p2i;

     /* Update filter results */
      p4r = p3r; p3r = p2r; p2r = p1r; p1r = p0r;
      p4i = p3i; p3i = p2i; p2i = p1i; p1i = p0i;
  
     /*==========================================
      * Basilar membrane response
      * 1/ shift up in frequency first: 
      *    (u0r+i*u0i) * exp(i*tpt*cf*t) = (u0r+i*u0i) * (qcos + i*(-qsin))
      * 2/ take the real part only: bm = real(exp(j*wcf*kT).*u) * gain;
      *==========================================
      */
      (*bm)[t] = ( u0r * qcos + u0i * qsin ) * gain;
     

     /*====================================================
      * The basic idea of saving computational load:
      * cos(a+b) = cos(a)*cos(b) - sin(a)*sin(b)
      * sin(a+b) = sin(a)*cos(b) + cos(a)*sin(b)
      * qcos = cos(tpt*cf*t) = cos(tpt*cf + tpt*cf*(t-1))
      * qsin = -sin(tpt*cf*t) = -sin(tpt*cf + tpt*cf*(t-1))
      *====================================================
      */
      qcos = coscf * ( oldcs = qcos ) + sincf * qsin;
      qsin = coscf * qsin - sincf * oldcs;
   }
   return;
}


/* the following function is identical to the preceeding function except that
 * it allows you to process the input in chunks and save the coefficients 
 * between function calls 
 */
void gammatoneFilterChunk(float *x, float **bm, float cf, int fs, int nsamples,
			  float *curr_p1r, float *curr_p2r, float *curr_p3r,
			  float *curr_p4r, float *curr_p1i, float *curr_p2i,
			  float *curr_p3i, float *curr_p4i, float *curr_qcos,
			  float *curr_qsin)
{
   int t;
   float a, tpt, tptbw, gain;
   float p0r, p1r, p2r, p3r, p4r, p0i, p1i, p2i, p3i, p4i;
   float a1, a2, a3, a4, a5, u0r, u0i; /*, u1r, u1i;*/
   float qcos, qsin, oldcs, coscf, sincf;

  /*=========================================
   * Initialising variables
   *=========================================
   */
   tpt = ( M_PI + M_PI ) / fs;
   tptbw = tpt * erb ( cf ) * BW_CORRECTION;
   a = exp ( -tptbw );

  /* based on integral of impulse response */
   gain = ( tptbw*tptbw*tptbw*tptbw ) / 3;

  /* Update filter coefficients */
   a1 = 4.0*a; a2 = -6.0*a*a; a3 = 4.0*a*a*a; a4 = -a*a*a*a; a5 = a*a;
   p0r = 0.0; p0i = 0.0;
   if (curr_p1r == NULL){
	   /* this is the very first chunk */
	   p1r = 0.0; p2r = 0.0; p3r = 0.0; p4r = 0.0;
	   p1i = 0.0; p2i = 0.0; p3i = 0.0; p4i = 0.0;
	   qcos = 1; qsin = 0;   /* t=0 & q = exp(-i*tpt*t*cf)*/
   } else {
	   p1r = *curr_p1r; p2r = *curr_p2r; p3r = *curr_p3r; p4r = *curr_p4r;
	   p1i = *curr_p1i; p2i = *curr_p2i; p3i = *curr_p3i; p4r = *curr_p4i;
	   qcos = *curr_qcos; qsin = *curr_qsin;
   }
 
  /*===========================================================
   * exp(a+i*b) = exp(a)*(cos(b)+i*sin(b))
   * q = exp(-i*tpt*cf*t) = cos(tpt*cf*t) + i*(-sin(tpt*cf*t))
   * qcos = cos(tpt*cf*t)
   * qsin = -sin(tpt*cf*t)
   *===========================================================
   */
   coscf = cos ( tpt * cf );
   sincf = sin ( tpt * cf );
   
   for ( t=0; t<nsamples; t++ )
   {
     /* Filter part 1 & shift down to d.c. */
      p0r = qcos*x[t] + a1*p1r + a2*p2r + a3*p3r + a4*p4r;
      p0i = qsin*x[t] + a1*p1i + a2*p2i + a3*p3i + a4*p4i;

     /* Clip coefficients to stop them from becoming too close to zero */
      if (fabs(p0r) < VERY_SMALL_NUMBER)
        p0r = 0.0F;
      if (fabs(p0i) < VERY_SMALL_NUMBER)
        p0i = 0.0F;
      
     /* Filter part 2 */
      u0r = p0r + a1*p1r + a5*p2r;
      u0i = p0i + a1*p1i + a5*p2i;

     /* Update filter results */
      p4r = p3r; p3r = p2r; p2r = p1r; p1r = p0r;
      p4i = p3i; p3i = p2i; p2i = p1i; p1i = p0i;
  
     /*==========================================
      * Basilar membrane response
      * 1/ shift up in frequency first: 
      *    (u0r+i*u0i) * exp(i*tpt*cf*t) = (u0r+i*u0i) * (qcos + i*(-qsin))
      * 2/ take the real part only: bm = real(exp(j*wcf*kT).*u) * gain;
      *==========================================
      */
      (*bm)[t] = ( u0r * qcos + u0i * qsin ) * gain;

     /*====================================================
      * The basic idea of saving computational load:
      * cos(a+b) = cos(a)*cos(b) - sin(a)*sin(b)
      * sin(a+b) = sin(a)*cos(b) + cos(a)*sin(b)
      * qcos = cos(tpt*cf*t) = cos(tpt*cf + tpt*cf*(t-1))
      * qsin = -sin(tpt*cf*t) = -sin(tpt*cf + tpt*cf*(t-1))
      *====================================================
      */
      qcos = coscf * ( oldcs = qcos ) + sincf * qsin;
      qsin = coscf * qsin - sincf * oldcs;
   }

   /* Update the tracked values */
   *curr_p1r = p1r; *curr_p2r = p2r; *curr_p3r = p3r; *curr_p4r = p4r;
   *curr_p1i = p1i; *curr_p2i = p2i; *curr_p3i = p3i; *curr_p4i = p4i;
   *curr_qcos = qcos; *curr_qsin = qsin;

   return;
}



