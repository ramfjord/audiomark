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

extern int debug;
extern watermark *w;
int embed(char *infile_path, char *outfile_path);

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

  embed(infile_path, outfile_path);

  print_watermark_info();
} // main

int embed(char *infile_path, char *outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;

  seed_rand(w->key_seed);
  set_rand(w->w_len); 

  //
  // Open input file and output file
  // -------------------------------

  // The pre-open SF_INFO should have format = 0, everything else will be set in the open call
  sfinfo.format = 0;
  
  // s_infile = input audio file
  if(!(s_infile = sf_open(infile_path, SFM_READ, &sfinfo))){
    fprintf(stderr,"error opening the following file for reading: %s\n", infile_path);
    return 1;
  }

  // s_outfile = watermaked audio file
  if(!(s_outfile = sf_open(outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile_path);
    return 1;
  }

  print_sfile_info(sfinfo);

  //
  // Read, process, and write data
  // -----------------------------

  time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));
  
  ft_forward = fftw_plan_dft_r2c_1d(BUFFER_LEN, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(BUFFER_LEN, freq_buffer, time_buffer, FFTW_ESTIMATE);

  int bytes_read;

  int counter = 0;
  double bytes_avg;
  while(bytes_read = sf_read_double(s_infile, time_buffer, BUFFER_LEN)){
    debug = (counter <= 3) ? 1 : 0;
    counter++;
    if(debug) printf("%d embed: ",counter);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < BUFFER_LEN; i++){
	bytes_avg += time_buffer[i] / BUFFER_LEN;
      }
    }
    if(bytes_read == BUFFER_LEN){
      fftw_execute(ft_forward);
      //process_signal(freq_buffer);
      fftw_execute(ft_reverse);
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);
    } //if

    sf_write_double(s_outfile, time_buffer, bytes_read);

  } //while

  SNDFILE *sfile;
  if(!(sfile = sf_open("test.wav", SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", "test.wav");
    return 1;
  }
  printf("looped %d times, avg num 3 = %f\n", counter, bytes_avg);
  for(int i = 0; i < BUFFER_LEN; i++)
    time_buffer[i] = 50;
  for(int i = 0; i < counter; i++)
    sf_write_double(sfile, time_buffer, BUFFER_LEN);
  sf_close(sfile);


  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  free_rand();
  sf_close(s_infile);
  sf_close(s_outfile);
/*}}}*/
} //embed
