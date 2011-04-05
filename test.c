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

int	test_debug = 1;
extern	watermark *w;

double	extract_wmark_elt(complex orig, complex test);
void	ss_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer, double *test_noise);
void	fh_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer);
int	test(char *orig_path, char *test_path);

int main(int argc, char *argv[]){
  char		orig_path[PATH_SIZE];
  char		test_path[PATH_SIZE];
  char		config_path[PATH_SIZE];

  FILE		*config;
  w = (watermark *)malloc(sizeof(watermark));

  w->w_message = NULL;
  w->alpha = .1;
  w->schema = MULT_SCHEMA;
  w->key_seed = 15;

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

  if(ERROR == parse_config(config_path)){
    fprintf(stderr, "Error parsing config");
    goto freedom;
  }
    

  if(w->w_message == NULL){
    printf("using default message\n");
    w->w_message = (char *)malloc(strlen("hello I'm thomas"));
    strcpy(w->w_message,"hello I'm thomas");
    w->w_len = strlen("hello I'm thomas");
  }

  //
  // test for watermark
  //
  test(orig_path, test_path);

  print_watermark_info();

freedom:
  free(w->w_message);
  free(w);
} // main

double extract_wmark_elt(complex orig, complex test){
/*{{{*/
  double orig_pow = sqrt(pow(creal(orig),2)+pow(cimag(orig),2));
  double test_pow = sqrt(pow(creal(test),2)+pow(cimag(test),2));

  switch (w->schema) {
   case PLUS_SCHEMA:
     return (test_pow - orig_pow) / w->alpha;
   case MULT_SCHEMA:
     return ((test_pow / orig_pow) - 1) / w->alpha;
   case POWR_SCHEMA:
    printf("UNIMPLEMENTED!!!");
    return 0;
   default:
    return 0;
  }
/*}}}*/
}

//extracts the watermark from the test_freq_buffer, given the orig_freq_buffer, and the length of the watermark
void fh_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer){
/*{{{*/
  int cur_elt;
  for(int i = 0; i < w->w_len; i++){
    cur_elt = next_rand(i);
    if(test_debug)
      printf("%5.d ", cur_elt);
    extract_buffer[i] += extract_wmark_elt(orig_freq_buffer[cur_elt], test_freq_buffer[cur_elt]);
  }
  if(test_debug)
    putchar('\n');
/*}}}*/
}

void ss_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer, double *test_noise){
/*{{{*/
  int		f_buffer_len = BUFFER_LEN / 2 + 1;
  double	noise_seq[f_buffer_len];

  generate_noise(noise_seq, f_buffer_len);
  embed_to_noise(w->w_message, noise_seq, f_buffer_len);

  double tmp;
  for(int i = 0; i < f_buffer_len; i++){
    tmp = extract_wmark_elt(orig_freq_buffer[i], test_freq_buffer[i]);
    
    // to indicate a strong correlation, test_seq and extract_buffer should match
    // There is a strong correlation in this frame if noise_seq matches the extracted watermark elements
    //if(noise_seq[i] < 0) tmp *= -1;
    if(test_noise[i] < 0) tmp *= -1;
    extract_buffer[i] += tmp;
  }
/*}}}*/
}

int test(char *orig_path, char *test_path){
/*{{{*/
  int	time_buffer_len = BUFFER_LEN;
  int	freq_buffer_len = time_buffer_len/2 + 1;

  char		*wmark  = w->w_message;
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

  // orig_wmark: the string that will be matched to the extracted watermark
  // extract buffer: the watermark we extract
  //
  // if a spread spectrum encoding is used, orig_wmark will be a random array, and in ss_extract_watermark() a matching 
  // correlation will move the extract_buffer closer to orig_wmark.
  //
  // if a frequency hopping encoding is used, orig_wmark will contain the watermarked string, and the extract buffer will
  // be the un-frequency hopped average difference between the test frames and the original frames.  These should match if
  // a watermark was embedded
  double	*extract_buffer;
  double	*orig_wmark;

  if(w->type == SS_EMBED){
    orig_wmark = (double *)malloc(sizeof(double) * freq_buffer_len);
    generate_noise(orig_wmark, BUFFER_LEN/2 + 1);
  }
  else{
    orig_wmark = (double *)malloc(sizeof(double) * strlen(wmark));
    for(int i = 0; i < strlen(wmark); i++){
      orig_wmark[i] = c_to_d(wmark[i]);
    }
  }

  // set up random number generator
  seed_rand(w->key_seed);
  set_rand(w->w_len); 

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


  orig_time_buffer = (double *) malloc(sizeof(double) * time_buffer_len);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * freq_buffer_len);

  test_time_buffer = (double *) malloc(sizeof(double) * time_buffer_len);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * freq_buffer_len);

  extract_buffer = (double *) malloc(sizeof(double) * freq_buffer_len);
  memset(extract_buffer, 0, sizeof(double)*freq_buffer_len);
  
  ft_orig = fftw_plan_dft_r2c_1d(BUFFER_LEN, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(BUFFER_LEN, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  int orig_bytes_read;
  int test_bytes_read;

  int counter = 0;
  while(1){
    test_debug = counter <= 3 ? 1 : 0;
    counter++;
    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, time_buffer_len);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, time_buffer_len);

    // no information will be encoded in the last "frame" - if we don't read in time_buffer_len
    // bytes, then there will be no embedded information there
    if(test_bytes_read != time_buffer_len || orig_bytes_read != time_buffer_len)
      break;

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    if(counter == 3 && test_debug){
      print_pow_density(orig_freq_buffer, 8);
      print_pow_density(test_freq_buffer, 8);
    }
    if(w->type == FH_EMBED)
      fh_extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer);
    else
      ss_extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer, orig_wmark);
  }

  // counter frames have been added together in the extract buffer
  // the correlation would be the same without dividing, but I like to be able to see the difference
  // between the extract_buffer and the orig_wmark more easily, so this averages out the frames
  // array_div(counter, extract_buffer, freq_buffer_len);

  double x_dot_x = 0, o_dot_x = 1;
  if(w->type != SS_EMBED){
    for(int i = 0; i < strlen(wmark); i++){
      x_dot_x += extract_buffer[i] * extract_buffer[i];
      o_dot_x += orig_wmark[i] * extract_buffer[i];
    }
  }
  else {
    //
    // let's debug this shit!
    //
    double max_diff = 0;
    int max_i = 0;
    for(int i = 0; i < freq_buffer_len; i++){
      x_dot_x += extract_buffer[i] * extract_buffer[i];
      o_dot_x += orig_wmark[i] * extract_buffer[i];

      if(extract_buffer[i] - orig_wmark[i] > max_diff){
	max_diff = extract_buffer[i] - orig_wmark[i];
	max_i = i;
      }
      if(orig_wmark[i] - extract_buffer[i] > max_diff){
	max_diff = orig_wmark[i] - extract_buffer[i];
	max_i = i;
      }
    }
    printf("max_diff = %f, i = %d\n", max_diff, max_i);
    for(int i = max_i - 5; i < max_i + w->processing_gain; i++)
      if(i >= freq_buffer_len) break;
      else
	printf("%5.2f\t", orig_wmark[i]);
    putchar('\n');
    for(int i = max_i - 5; i < max_i + w->processing_gain; i++)
      if(i >= freq_buffer_len) break;
      else
	printf("%5.2f\t", extract_buffer[i]);
  }

  puts("\norig, then test");
  int num_to_print;
  if(w->type == FH_EMBED)
    num_to_print = w->w_len;
  else
    num_to_print = 20;
  for(int i = 0; i < num_to_print; i++){
    printf("%5.2f\t",orig_wmark[i]);
  }
  putchar('\n');
  for(int i = 0; i < num_to_print; i++){
    printf("%5.2f\t",extract_buffer[i]);
  }
  putchar('\n');

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
/*}}}*/
}
