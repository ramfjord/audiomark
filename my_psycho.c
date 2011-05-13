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
#include "w_array_ops.h"

double cb_limits[25] = { 20,  100,  200,  300,  400,  510,  630,  770,
										 	  920, 1080, 1270, 1480, 1720, 2000, 2320, 2700,
										 	 3150, 3700, 4400, 5300, 6400, 7700, 9500,
										  12000, 15500};

double min_amplitude = 0;

double hz_to_bark(double hz)
{/*{{{*/
	// according to terhardt
	return 13 * atan(.00076*hz ) + 3.5*atan( (hz/7500.0)*(hz/7500.0) );
	// according to Prof Ambikairajah...
	//if(hz > 1500)
	//	return 8.7 + 14.2*log10(hz);
	//return 13.0 * atan(.76 * hz);
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

// Returns the closest index in the frequency buffer to freq
int get_i(double freq)
{ //{{{
	int i = (int)(freq * 1024 / SAMPLE_RATE + .5);
	if(i < 0) i = 0;
	if(i >= FREQ_LEN) i = FREQ_LEN-1;

	return i;
} //}}}

// Returns an estimate of the power of the amplitude of the frequency component
// in dB.  because you need a reference amplitude, I use the minimum amplitude
// in the current frequency frame.  Not sure how sound this is.
// NOTE: IF CALLED BEFORE get_amp_dB, will generate runtime exception for
// dividing by 0
double get_dB(const complex a)
{ //{{{
	return 20 * log10(sqrt( pow(creal(a),2) + pow(cimag(a),2) ) / min_amplitude);
} //}}}

// amp_dB is set to the amplitudes of freq_buf, in dB
void get_amp_dB(const complex *freq_buf, double *amp_dB)
{ //{{{
	min_amplitude = 0; // global
	for(int i = 0; i < FREQ_LEN; i++)
		if(pow_elem(freq_buf[i]) < min_amplitude) 
			min_amplitude = pow_elem(freq_buf[i]);

	for(int i = 0; i < FREQ_LEN; i++)
		amp_dB[i] = get_dB(freq_buf[i]);
} //}}}

// Terhardt describes the following method of determining whether a component
// is tonal. This implementation assumes amp_db is indexed such that the
// component to be examined is at index 0
// TODO test me
int is_tonal(const double *amp_dB)
{ // {{{
	double L_i = *amp_dB; // name used in Terhardt

	if(amp_dB[1] < L_i) return 0;
	if(amp_dB[-1] <= L_i) return 0;

	for(int i = -3; i <= 3; i++)
		if(i != 1 && i != -1 && L_i - amp_dB[i] < 7) return 0;

	return 1;
} // }}}

// iterates through freq_buf, storing the indices of the tonal components in
// indices. returns the number of tonal components. indices is sorted.
// TODO test me
int get_tonal_freqs(const double *amp_dB, int **indices)
{ //{{{
	// this is the maximum amount of tonal frequencies.  Not actually dynamically
	// sizing it now, but leaving open the possibility.
	*indices = (int *)malloc(sizeof(int) * (FREQ_LEN/7));

	int i_i = 0; // i_i = indices index
	// tonal calculations look at i-3...i+3, also don't want nyquist/dc freqs
	for(int i = 4; i < FREQ_LEN-4; i++){ 
		if(is_tonal(&amp_dB[i])) (*indices)[i_i++] = i;
	}

	return i_i;
} //}}}

// generates the sum of L_Ev components in Terhardt (4)
// see also (5), (7)
double get_inter_mask(double *amp_dB, int *tonal_indices, int num_tonal, int u)
{ //{{{
	// TODO only go through TONAL INDICES
	double f_u = get_freq(u);
	double z_u = hz_to_bark(f_u);
	double sum = 0;
	
	// calculate the masking effect from other frequencies by using a linear
	// approximation of the masking curve
	for(int v = 1; v < FREQ_LEN; v++){
		double L_v = amp_dB[v];
		double f_v = get_freq(v);
		double z_v = hz_to_bark(f_v);

		double s; // slope of masking curve is different for above/below v
		if(v < u)
			s = -24.0 - (230.0/f_v) + 0.2*L_v; // slope of masking curve above masker
		else if(v > u)
			s = 27; // slope below masker
		else continue;

	  double exp = (L_v - s * (z_v - z_u))/20.0;
		sum += pow(10,exp);
	}

	return sum*sum;
} //}}}

double get_intra_mask(complex *freq_buf, int f, int cb)
{ //{{{
	int bandwidth = 0;  // the number of frequencies within the critical band
	while((f+bandwidth < FREQ_LEN) && get_freq(f+bandwidth) <= cb_limits[cb])
		bandwidth++;

	// get tonal components
	for(int i = 0; i < bandwidth; i++) ;

} //}}}

// equation (8) in terhardt
// TODO test me
double get_quiet_thresh(double f)
{ //{{{
	return   3.64*pow(f/1000,-0.8) 
				 - 6.5*exp((-0.6*pow(f/(1000-3.3),2))) 
				 + .001*pow(f/1000,4);
} //}}}

// returns the masking effect of the noise (non-tonal) components around u, 
// I_Nu
double get_noise_mask(double *amp_dB, int *tonal_indices, int num_tonal, int u)
{ //{{{
	// Sum all the frequency components within .5 barks of u, skipping the five
	// central samples of each tonal component (if i is tonal, no i-2...i+2)
	int u_hz = get_freq(u);
	int u_bark = hz_to_bark(u_hz);
	double cb_width = cb_limits[(int)(u_bark+1)]; // upper bound on freq width

	int lo = get_i(u_hz - (cb_width/2));
	while(u_hz - hz_to_bark(get_freq(lo)) > .5) lo++;

	int hi = get_i(u_hz + (cb_width/2));
	while(hz_to_bark(get_freq(hi)) - u_hz > .5) hi--;

	int i_i = 0;
	double sum = 0;
	for(int i = lo; i <= hi; i++){
		// ensure tonal_indices[i_i] >= i-2 now
		while(i+i < num_tonal && tonal_indices[i_i] < i-2){
			i_i++;
		}

		// no tonal components above i-2
		if(i_i >= num_tonal){
			sum += amp_dB[i];
			continue;
		}

		if(tonal_indices[i_i] > i+2){
			sum += amp_dB[i];
		}
		// otherwise tonal_indices[i_i] <= i+2, and we're in a tonal component
	}

	return sum;
} //}}}

int get_masking_coeffs(complex *buffer, int **indices)
{ //{{{
	// CALCULATION OF SIMULTANEOUS MASKING COEFFICIENTS

	// see "Algorithm for extraction of pitch and pitch salience from
	// complex tonal signals", by Terhardt, Stoil, Seewann

	double amp_dB[FREQ_LEN]; // the amplitude of each freq component in dB
	get_amp_dB(buffer,amp_dB);

	// calculate tonal simultaneous masking
	int *tonal_indices;
	int num_tonal = get_tonal_freqs(amp_dB, &tonal_indices);

	// iterate through tonal frequency components, and calculate the sound
	// pressure level excess of each one.
	double *spl_excess = (double *)malloc(num_tonal * sizeof(double));
	double intra_mask = 0;
	for(int j = 0; j < num_tonal; j++){
		int i = tonal_indices[j];

		// calculate frequency and critical band of the component

		// effect of other frequencies on spl_excess
		double inter_mask = get_inter_mask(amp_dB, tonal_indices, num_tonal, i);

		// effect of non-tonal components on spl_excess.
		double noise_mask = get_noise_mask(amp_dB, tonal_indices, num_tonal, i);

		// effect of the threshold of quiet - even in silence a frequency of this
		double freq = get_freq(i);
		double q_mask = get_quiet_thresh(freq);
		q_mask = pow(10,q_mask/10);

		// see (4)
		spl_excess[i] - amp_dB[i] - (10*log(inter_mask + noise_mask + q_mask));
	}

	// Temporal masking: ~1:25 Ambikairajah
	//  Jesteadt et al. : M_f(t,m) = a*(b-log_10 dt)(L(t,m)-c)
	// a, b, c are parameters that can be derived from psychoacoustic data
	// a is based on the slope of the time course of masking
	// b = log_10(forward masking duratioin = 200)


	// Masking threshold = (TM^P + SN^P)^{1/P},
	// where TM = temporal masking, SM = simultaneous masking
	// often P = 5 seems to work pretty well

	free(tonal_indices);
	free(spl_excess);
	
	// TODO change return value
	return 1;
} //}}}

int main()
{
	int blah;
}
