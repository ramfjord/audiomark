/*---------------------------------------------------------------------
 * Psychoacoustic model
 *
 * Tonal masking based on "Algorithm for extraction of pitch and pitch
 * salience from complex tonal signals", Terhardt et al.
 *
 * Non-tonal masking taken from Ambikairajah
 *--------------------------------------------------------------------*/

#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <sndfile.h>
#include "watermark.h"
#include "my_psycho.h"

double cb_limits[25] = { 20,  100,  200,  300,  400,  510,  630,  770,
										 	  920, 1080, 1270, 1480, 1720, 2000, 2320, 2700,
										 	 3150, 3700, 4400, 5300, 6400, 7700, 9500,
										  12000, 15500};

double hz_to_bark(double hz)
{/*{{{*/
//return 13 * atan(.00076*hz ) + 3.5*arctan( (hz/7500.0)*(hz/7500.0) );
	// according to Prof Ambikairajah...
	if(hz > 1500)
		return 8.7 + 14.2*log10(hz);
	return 13.0 * atan(.76 * hz);
}/*}}}*/

int get_critband(double hz)
{ //{{{
	// band 0 doesn't exist, and should be cut off
	int i;
	for(i = 0; i < 25; i++)
		if(hz <= cb_limits[i]) return i;
	return i;
} //}}}

// Returns the frequency of the component at index i, in the complex buffer
// from the fft
double get_freq(int i)
{ //{{{
	return (double)i * SAMPLE_RATE / 1024;
} //}}}

// Returns an estimate of the power of the amplitude of the frequency component
// in dB
double get_dB(complex a)
{ //{{{
	// TODO possibly find a better version of this
	return 20 * log10(sqrt( pow(creal(a),2) + pow(cimag(a),2) ) / MIN_THRESH);
} //}}}

// The process fore generating the interband mask is described in Terhardt,
// as well as in Ambikairajah's lecture at 1:08
double get_inter_mask(complex *freq_buf, int u)
{ //{{{
	double f_u = get_freq(u);
	double z_u = hz_to_bark(f_u);
	double sum = 0;
	
	// calculate the masking effect of all the frequencies below v
	for(int v = 1; v < u; v++){
		double L_v = get_dB(freq_buf[v]); // masker power in db
		double f_v = get_freq(v);
		double z_v = hz_to_bark(f_v);
		double s_vh = -24.0 - (230.0/f_v) + 0.2*L_v; // slope of masking curve 
																								 // above the masker

	  double exp = (L_v - s_vh * (z_v - z_u))/20.0;
		sum += pow(10,exp);
	}

	// calculate the masking effect for all the frequencies above v
	double s_vl = 27; // The slope of the masking curve below the masker
	for(int v = u; v < FREQ_LEN; v++){
		double L_v = get_dB(freq_buf[v]);
		double f_v = get_freq(v);
		double z_v = hz_to_bark(f_v);

	  double exp = (L_v - s_vl*(z_v - z_u))/20.0;
		sum += pow(10,exp);
	}

	return 20 * log(sum);
} //}}}

double get_intra_mask(complex *freq_buf, int f, int cb)
{ //{{{
	int bandwidth = 0;  // the number of frequencies within the critical band
	while((f+bandwidth < FREQ_LEN) && get_freq(f+bandwidth) <= cb_limits[cb])
		bandwidth++;

	// get tonal components
	for(int i = 0; i < bandwidth; i++) ;

} //}}}

int get_masking_coeffs(complex *buffer, double *coeffs[2])
{ //{{{
	// CALCULATION OF SIMULTANEOUS MASKING COEFFICIENTS

	// see "Algorithm for extraction of pitch and pitch salience from
	// complex tonal signals", by Terhardt, Stoil, Seewann

	// iterate through frequency components, and calculate the minimum amplitude
	// necessary for it to be heard.  Recall that in an fft, the component at
	// index 0 is just the average of the others.
	int cb = 0; 
	double intra_mask = 0;
	for(int i = 1; i < FREQ_LEN; i++){
		// calculate frequency and critical band of the component
		double freq = get_freq(i);

		if(freq > cb_limits[cb]){
			// The intraband mask is the same over the entier critical band
			// as we enter a new critical band, calculate the intra_mask, and
			// use it until we get to the next critical band
			cb++;
			//intra_mask = get_intra_mask(&(buffer[i]), cb);
		}

		// calculate the threshold of quiet - even in silence a frequency of this
		// amplitude will not be heard.
		//double q_mask = get_quite_thresh(freq);
		
		// look up Gammatone filte

		double inter_mask = get_inter_mask(buffer, i);
	}

	// Temporal masking: ~1:25 Ambikairajah
	//  Jesteadt et al. : M_f(t,m) = a*(b-log_10 dt)(L(t,m)-c)
	// a, b, c are parameters that can be derived from psychoacoustic data
	// a is based on the slope of the time course of masking
	// b = log_10(forward masking duratioin = 200)


	// Masking threshold = (TM^P + SN^P)^{1/P},
	// where TM = temporal masking, SM = simultaneous masking
	// often P = 5 seems to work pretty well
} //}}}

int main()
{
	int blah;
}
