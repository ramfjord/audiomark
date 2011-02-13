
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

void embed_to_noise(char *wmark, double *noise_seq, int buffer_len);
int ssembed(char *infile_path, char *outfile_path);

extern watermark *w;
extern int debug;

int main(int argc, char *argv[]){
  debug = 0;

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

  ssembed(infile_path, outfile_path);

  print_watermark_info();
} // main


int ssembed(char *infile_path, char *outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  int		f_buffer_len = w->processing_gain * w->w_len * 8;
  int		t_buffer_len = (f_buffer_len-1) * 2;

  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;

  double	noise_seq[f_buffer_len];

  srand(w->key_seed);

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
  printf("input file: %s\n",infile_path);

  // s_outfile = watermaked audio file
  if(!(s_outfile = sf_open(outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile_path);
    return 1;
  }
  printf("watermarked file: %s\n", outfile_path);

  printf("Input file info:\n");
  print_sfile_info(sfinfo);

  //
  // Read, process, and write data
  // -----------------------------

  time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);
  
  ft_forward = fftw_plan_dft_r2c_1d(t_buffer_len, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(t_buffer_len, freq_buffer, time_buffer, FFTW_ESTIMATE);

  int bytes_read;

  int counter = 0;
  double bytes_avg;
  while(bytes_read = sf_read_double(s_infile, time_buffer, t_buffer_len)){
    //debug = (counter <= 3) ? 1 : 0;
    counter++;

    if(debug) printf("%d embed: ",counter);

    if(bytes_read == t_buffer_len){
      if(counter == 2){
	printf("pre-watermark sample:\n");
	print_d_array(time_buffer, 10);
      }

      fftw_execute(ft_forward);

      if(counter == 2)
	print_r_complex_array(freq_buffer,10);

      generate_noise(noise_seq, f_buffer_len);
      embed_to_noise(wmark, noise_seq, f_buffer_len);
      //////complex_array_add(freq_buffer, noise_seq, f_buffer_len);
      d_array_to_complex(freq_buffer, noise_seq, f_buffer_len);
      ////print_d_array(noise_seq, f_buffer_len);

      if(counter == 2)
	print_r_complex_array(freq_buffer, 10);

      fftw_execute(ft_reverse);
      array_div(t_buffer_len, time_buffer, t_buffer_len);

      if(counter == 2){
      printf("post-watermark sample:\n");
      print_d_array(time_buffer, 10);
      }
    } //if

    sf_write_double(s_outfile, time_buffer, bytes_read);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < t_buffer_len; i++){
	bytes_avg += time_buffer[i] / t_buffer_len;
      }
      print_d_array(noise_seq, 10);
    }
  } //while

  //SNDFILE *sfile;
  //if(!(sfile = sf_open("test.wav", SFM_WRITE, &sfinfo))){
  //  fprintf(stderr,"error opening the following file for writing: %s\n", "test.wav");
  //  return 1;
  //}
  //printf("looped %d times, avg num 3 = %f\n", counter, bytes_avg);
  //for(int i = 0; i < t_buffer_len; i++)
  //  time_buffer[i] = 50;
  //for(int i = 0; i < counter; i++)
  //  sf_write_double(sfile, time_buffer, t_buffer_len);
  //sf_close(sfile);


  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  sf_close(s_infile);
  sf_close(s_outfile);
/*}}}*/
} //embed
