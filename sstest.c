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

int sstest(char *infile_path, char *outfile_path);

extern watermark *w;
extern int debug;

int main(int argc, char *argv[]){
  char		infile_path[PATH_SIZE];
  char		outfile_path[PATH_SIZE];
  char		config_path[PATH_SIZE];

  FILE		*config;
  w = (watermark *)malloc(sizeof(watermark));

  w->w_message = NULL;
  w->alpha = .1;
  w->schema = MULT_SCHEMA;
  w->key_seed = 15;

  // set default parameters for testing
  strcpy(infile_path, "input.wav");
  strcpy(outfile_path, "out.wav");
  strcpy(config_path, "wmark.cfg");

  //
  // Parse parameters
  //
  if(argc > 1)
    strncpy(infile_path, argv[1], PATH_SIZE);
  if(argc > 2)
    strncpy(outfile_path, argv[2], PATH_SIZE);
  if(argc > 3)
    strncpy(config_path, argv[3], PATH_SIZE);

  parse_config(config_path);

  if(w->w_message == NULL){
    printf("using default message\n");
    w->w_message = (char *)malloc(strlen("hello I'm thomas"));
    strcpy(w->w_message,"hello I'm thomas");
    w->w_len = strlen("hello I'm thomas");
  }

  sstest(infile_path, outfile_path);

  print_watermark_info();
} // main

int sstest(char *orig_path, char *test_path){
/*{{{*/

  char		*wmark = w->w_message;
  int		f_buffer_len = w->processing_gain * w->w_len * 8;
  int		t_buffer_len = (f_buffer_len-1) * 2;

  SF_INFO	sfinfo_orig;		// struct with info on the samplerate, number of channels, etc
  SF_INFO	sfinfo_test;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_orig;
  SNDFILE   	*s_test;

  fftw_plan 	ft_orig;
  fftw_plan 	ft_test;
  double	*orig_time_buffer;
  complex	*orig_freq_buffer;

  double	*test_time_buffer;
  complex	*test_freq_buffer;

  double	noise_seq[f_buffer_len];
  //double	extract_buffer[w->w_len * 8];
  double	despread_buffer[f_buffer_len];

  srand(w->key_seed);

  sfinfo_orig.format = 0;
  if(!(s_orig = sf_open(orig_path, SFM_READ, &sfinfo_orig))){
    fprintf(stderr,"error opening the following file for reading: %s\n", orig_path);
    return 1;
  }
  printf("un-watermarked file: %s\n",orig_path);


  sfinfo_test.format = 0;
  if(!(s_test = sf_open(test_path, SFM_READ, &sfinfo_test))){
    fprintf(stderr,"error opening the following file for reading: >%s<\n", test_path);
    return 1;
  }
  printf("file to be tested: %s\n", test_path);


  orig_time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);

  test_time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);

  //extract_buffer = (double *) malloc(sizeof(double) * f_buffer_len);
  
  ft_orig = fftw_plan_dft_r2c_1d(t_buffer_len, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(t_buffer_len, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  int orig_bytes_read;
  int test_bytes_read;

  int counter = 0;

  printf("unwatermarked file info:\n");
  printf("\tsamplerate: %d, %d\n", sfinfo_orig.samplerate, sfinfo_test.samplerate);
  printf("\tchannels: %d, %d\n", sfinfo_orig.channels, sfinfo_test.channels);
  printf("\tsections: %d, %d\n", sfinfo_orig.sections, sfinfo_test.sections);
  printf("\tformat: %d, %d\n", sfinfo_orig.format, sfinfo_test.format);
  printf("\tframes: %d, %d\n", (int)sfinfo_orig.frames, (int)sfinfo_test.frames);
  printf("\tseekable: %d, %d\n", sfinfo_orig.seekable, sfinfo_test.seekable);

  double bytes_avg;
  double o_dot_x;
  double x_dot_x;
  while(1){
    debug = (counter <= 3) ? 1 : 0;
    counter++;
    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, BUFFER_LEN);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, BUFFER_LEN);

    if(test_bytes_read != BUFFER_LEN || orig_bytes_read != BUFFER_LEN)
      break;

    if(counter == 2){
      printf("un-watermark sample:\n");
      print_d_array(orig_time_buffer, 10);
      printf("test sample:\n");
      print_d_array(test_time_buffer, 10);
    }

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    generate_noise(noise_seq, f_buffer_len);
    embed_to_noise(wmark, noise_seq, f_buffer_len);

    if(counter == 2){
      print_r_complex_array(orig_freq_buffer, 10);
      print_r_complex_array(test_freq_buffer, 10);
    }

    for(int i = 0; i < f_buffer_len; i++){
      test_freq_buffer[i] -= orig_freq_buffer[i];
      o_dot_x += creal(test_freq_buffer[i]) * noise_seq[i];
      x_dot_x += creal(test_freq_buffer[i]) * creal(test_freq_buffer[i]);
    }

    if(counter == 2){
      print_r_complex_array(test_freq_buffer, 10);
      print_d_array(noise_seq, 10);
    }
  } //while

  printf("sim(W,W*) = %f\n", o_dot_x / sqrt(x_dot_x));

  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_orig);
  fftw_destroy_plan(ft_test);
  fftw_free(orig_freq_buffer);
  fftw_free(test_freq_buffer);
  free(orig_time_buffer);
  free(test_time_buffer);
/*}}}*/
} //embed
