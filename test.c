#ifndef W_INCLUDES
#define W_INCLUDES

#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <sndfile.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "watermark.h"
#include "wrandom.h"
#include "w_array_ops.h"

#endif

int	test_debug = 0;
extern	watermark *wmark;
int	test_iteration = 0;

double	extract_wmark_elt(complex orig, complex test);
int ss_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *ex_dot_orig, double *ex_dot_ex);
void	fh_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer);
int	test(char *orig_path, char *test_path);

int main(int argc, char *argv[])
{ //{{{
  char		orig_path[PATH_SIZE];
  char		test_path[PATH_SIZE];
  char		config_path[PATH_SIZE];

  FILE		*config;

  // set default parameters for testing
  strcpy(orig_path, "audio/input.wav");
  strcpy(test_path, "audio/out.wav");
  strcpy(config_path, "wmark.cfg");

  //
  // Parse parameters
  //
  if(argc > 1)
    strncpy(orig_path, argv[1], PATH_SIZE);
  if(argc > 2)
    strncpy(test_path, argv[2], PATH_SIZE);
  if(argc > 3)
    strncpy(config_path, argv[3], PATH_SIZE);

	gen_default_wmark();
  if(ERROR == parse_config(config_path)){
    fprintf(stderr, "Error parsing config");
    goto freedom;
  }

  //
  // test for watermark
  //
  test(orig_path, test_path);

  print_watermark_info();

freedom:
	free_wmark();
} //}}}


double extract_wmark_elt(complex orig, complex test)
{ //{{{
	double orig_pow = pow_spect_dens(orig);
	double test_pow = pow_spect_dens(test);

	switch (wmark->schema) {
		case PLUS_SCHEMA:
			//return (test_pow - orig_pow) / wmark->alpha;
			return (creal(test)-creal(orig)+cimag(test)-cimag(orig)) / (2.0 * wmark->alpha);
		case MULT_SCHEMA:
			return ((test_pow / orig_pow) - 1) / wmark->alpha;
		case POWR_SCHEMA:
			return log(test_pow/orig_pow) / wmark->alpha;
			return 0;
		default:
			return 0;
	}
} //}}}

//extracts the watermark from the test_freq_buffer, given the orig_freq_buffer, and the length of the watermark
void fh_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer)
{ //{{{
    //if(test_iteration >= 4 && test_iteration <= 5 || test_iteration == 0)
    //  printf("test %d orig: ", test_iteration); print_pow_density(orig_freq_buffer,10);
  int next_r;
  double extracted_elt;
  for(int i = 0; i < wmark->len; i++){
    next_r = next_rand(i);
    extracted_elt = extract_wmark_elt(orig_freq_buffer[next_r], test_freq_buffer[next_r]);
    extract_buffer[i] += extracted_elt;

    double tmp = c_to_d(wmark->message[i]) - extracted_elt;
    if(tmp > .1 || tmp < -.1){
    //if(test_iteration >= 4 && test_iteration <= 5 || test_iteration == 0){
      complex old_freq_elt = orig_freq_buffer[next_r];
      complex new_freq_elt = test_freq_buffer[next_r];

      printf("test %d %c, %1.4f+%1.4fi => %1.4f+%1.4fi : %d : %f\n",
	  test_iteration, wmark->message[i],
	  creal(old_freq_elt), cimag(old_freq_elt),
	  creal(new_freq_elt), cimag(new_freq_elt),
	  next_r, extracted_elt);
    }
  }
} //}}}

//
// Watermark extraction sequence described in Cox et al. 1997
// orig_freq_buffer: the original frame in the frequency domain
// test_freq_buffer: the test frame in the freq domain
// o
//
int ss_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, 
												 double *ex_dot_orig, double *ex_dot_ex)
{ //{{{
  int		f_buffer_len = BUFFER_LEN / 2 + 1;

	// embed indices are the indices of V in Cox et al.: these are the indices in
	// which the waterark is actually embedded.
	int *embed_indices;
	int noise_len = extract_sequence_indices(orig_freq_buffer, f_buffer_len, 
																					 &embed_indices);
	// Here we are generating the original watermark message, noise_seq
	// corresponds to W in Cox et al.
  double	noise_seq[noise_len];
  generate_noise(noise_seq, noise_len);
  embed_to_noise(noise_seq, noise_len);

	// add what we've generated/extracted to the overall watermark.  Loop around.
	double orig_elt;
	double ex_elt;
  for(int i = 0; i < noise_len; i++){
    ex_elt = extract_wmark_elt(orig_freq_buffer[i], test_freq_buffer[i]);
		orig_elt = noise_seq[i];

		*ex_dot_ex += ex_elt * ex_elt;
		*ex_dot_orig += ex_elt * orig_elt;
  }

	free(embed_indices);
	return noise_len;
} //}}}

int test(char *orig_path, char *test_path)
{ //{{{
  int	time_buffer_len = BUFFER_LEN;
  int	freq_buffer_len = time_buffer_len/2 + 1;

  char		*message  = wmark->message;
  SF_INFO	sfinfo_orig;		// struct with info on the samplerate, number of channels, etc
  SF_INFO	sfinfo_test;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_orig;
  SNDFILE   	*s_test;

  fftw_plan 	ft_orig;
  fftw_plan 	ft_test;

  // time_buffers contain the time domain data extracted from the wav file
  // freq_buffers contain the frequency domain elements after the fourier transform
  // data is extracted from the freq buffers
  double	*orig_time_buffer;
  complex	*orig_freq_buffer;

  double	*test_time_buffer;
  complex	*test_freq_buffer;

  orig_time_buffer = (double *) malloc(sizeof(double) * time_buffer_len);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * freq_buffer_len);

  test_time_buffer = (double *) malloc(sizeof(double) * time_buffer_len);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * freq_buffer_len);

  // orig_wmark: the string that will be matched to the extracted watermark
  // extract buffer: the watermark we extract
  //
	// if a spread spectrum encoding is used, orig_wmark will be a random array,
	// and in ss_extract_watermark() a matching correlation will move the
	// extract_buffer closer to orig_wmark.
  //
	// if a frequency hopping encoding is used, orig_wmark will contain the
	// watermarked string, and the extract buffer will be the un-frequency hopped
	// average difference between the test frames and the original frames.  These
	// should match if a watermark was embedded
  int		extract_buffer_len;
  double	*extract_buffer;
  double	*orig_wmark;

  if(wmark->type == SS_EMBED){
    extract_buffer_len = wmark->len;

    orig_wmark = (double *)malloc(sizeof(double) * extract_buffer_len);
		memset(orig_wmark,0,wmark->len);
  }
  else{
    extract_buffer_len = strlen(message);

    orig_wmark = (double *)malloc(sizeof(double) * extract_buffer_len);
    for(int i = 0; i < extract_buffer_len; i++){
      orig_wmark[i] = c_to_d(message[i]);
    }
  }

  extract_buffer = (double *) malloc(sizeof(double) * extract_buffer_len);
  memset(extract_buffer, 0, sizeof(double)*extract_buffer_len);

  // set up random number generator
  seed_rand(wmark->key_seed);
  set_rand(wmark->len); 

  sfinfo_orig.format = 0;
  if(!(s_orig = sf_open(orig_path, SFM_READ, &sfinfo_orig))){
    fprintf(stderr,"error opening the following file for reading: %s\n", orig_path);
    return 1;
  }

  sfinfo_test.format = 0;
  if(!(s_test = sf_open(test_path, SFM_READ, &sfinfo_test))){
    fprintf(stderr,"error opening the following file for reading: >%s<\n", test_path);
    return 1;
  }

  // set up transform plans
  ft_orig = fftw_plan_dft_r2c_1d(BUFFER_LEN, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(BUFFER_LEN, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  // variables for while
  int orig_bytes_read;
  int test_bytes_read;
  int counter = 0;

  double x_dot_x = 0, o_dot_x = 0;
  while(1){
    // DEBUGGING
    // debug on first 4 iterations
    test_debug = counter <= 3 ? 1 : 0;
    //printf("iteration %d\n", test_iteration);
    //

    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, time_buffer_len);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, time_buffer_len);

    // no information will be encoded in the last "frame" - if we don't read in time_buffer_len
    // bytes, then there will be no embedded information there
    if(test_bytes_read != time_buffer_len || orig_bytes_read != time_buffer_len)
      break;

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    if(counter == 3 && test_debug){
      printf("org: "); print_pow_density(orig_freq_buffer, 10);
      printf("tst: "); print_pow_density(test_freq_buffer, 10);
    }

    if(wmark->type == FH_EMBED)
      fh_extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer);
    else
      ss_extract_watermark(orig_freq_buffer, test_freq_buffer, 
													 &o_dot_x, &x_dot_x);

    counter++;
    test_iteration++;
  } // while


  // counter frames have been added together in the extract buffer
  // the correlation would be the same without dividing, but I like to be able to see the difference
  // between the extract_buffer and the orig_wmark more easily, so this averages out the frames
  array_div(test_iteration, extract_buffer, extract_buffer_len);

  if(wmark->type != SS_EMBED){
    for(int i = 0; i < strlen(message); i++){
      x_dot_x += extract_buffer[i] * extract_buffer[i];
      o_dot_x += orig_wmark[i] * extract_buffer[i];
    }
  }
  //else {
	//	//
	//	// let's debug this shit!
	//	//
	//	double max_diff = 0;
	//	int max_i = 0;
	//	for(int i = 0; i < freq_buffer_len; i++){
	//		x_dot_x += extract_buffer[i] * extract_buffer[i];
	//		o_dot_x += orig_wmark[i] * extract_buffer[i];

	//		if(extract_buffer[i] - orig_wmark[i] > max_diff){
	//			max_diff = extract_buffer[i] - orig_wmark[i];
	//			max_i = i;
	//		}
	//		if(orig_wmark[i] - extract_buffer[i] > max_diff){
	//			max_diff = orig_wmark[i] - extract_buffer[i];
	//			max_i = i;
	//		}
	//	}
	//	printf("max_diff = %f, i = %d\n", max_diff, max_i);
	//	for(int i = max_i - 5; i < max_i + wmark->processing_gain; i++)
	//		if(i >= freq_buffer_len) break;
	//		else
	//			printf("%5.2f\t", orig_wmark[i]);
	//	putchar('\n');
	//	for(int i = max_i - 5; i < max_i + wmark->processing_gain; i++)
	//		if(i >= freq_buffer_len) break;
	//		else
	//			printf("%5.2f\t", extract_buffer[i]);
	//}

	//puts("\norig, then test");
	//int num_to_print;
	//if(wmark->type == FH_EMBED)
	//	num_to_print = wmark->len;
	//else
	//	num_to_print = 20;
	//for(int i = 0; i < num_to_print; i++){
	//	printf("%5.2f\t",orig_wmark[i]);
	//}
	//putchar('\n');
	//for(int i = 0; i < num_to_print; i++){
	//	printf("%5.2f\t",extract_buffer[i]);
	//}
	//putchar('\n');

	printf("W*.W* = %f, W.W* = %f\n", x_dot_x, o_dot_x);
	printf("sim(W,W*) = %f\n", o_dot_x / sqrt(x_dot_x));

  //
  // Freedom!
  //
  fftw_destroy_plan(ft_test);
  fftw_destroy_plan(ft_orig);

  fftw_free(orig_freq_buffer);
  fftw_free(test_freq_buffer);
  free(orig_time_buffer);
  free(test_time_buffer);

  free(orig_wmark);
  free(extract_buffer);

  sf_close(s_orig);
  sf_close(s_test);
  free_rand();
} //}}}
