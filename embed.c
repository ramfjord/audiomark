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

// pulled from watermark.c
extern	watermark *w;
int	embed_debug = 1;

// These functions are only used in embedding
void  print_embedding_info(char *infile_path, char *outfile_path, char *orig_out_path, char *config_path);
void  watermark_elt(double w_d, complex *freq_elt);
void  ss_process_signal(complex *freq_buffer);
void  fh_process_signal(complex *freq_buffer);
int   embed(char *infile_path, char *outfile_path, char *orig_outfile_path);

int main(int argc, char *argv[]){
  char		infile_path[PATH_SIZE];
  char		outfile_path[PATH_SIZE];
  char		config_path[PATH_SIZE];
  char		orig_out_path[PATH_SIZE];
  FILE		*config;

  w = (watermark *)malloc(sizeof(watermark));

  w->w_message = NULL;
  w->alpha = .1;
  w->schema = MULT_SCHEMA;
  w->key_seed = 15;

  // set default parameters for testing
  strcpy(infile_path, "audio/input.wav");
  strcpy(outfile_path, "audio/out.wav");
  strcpy(orig_out_path, "audio/orig_out.wav");
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

  if(ERROR == parse_config(config_path)){
    fprintf(stderr, "Error parsing config");
    goto freedom;
  }

  if(w->w_message == NULL){
    printf("using default message\n");
    w->w_message = (char *)malloc(strlen("hello I'm thomas"));
    strcpy(w->w_message,"hello I'm thomas");
    w->w_len = strlen("hello I'm thomas");
    w->type = SS_EMBED;
  }
  
  w->processing_gain = BUFFER_LEN / w->w_len;

  print_embedding_info(infile_path, outfile_path, orig_out_path, config_path);

  print_watermark_info();

  embed(infile_path, outfile_path, orig_out_path);

  print_watermark_info();

freedom:
  free(w->w_message);
  free(w);
} // main

void watermark_elt(double w_d, complex *freq_elt){
/*{{{*/
  switch (w->schema) {
   case PLUS_SCHEMA:
    *freq_elt += w->alpha * w_d;
    *freq_elt += w->alpha * w_d * I;
    return;
   case MULT_SCHEMA:
    *freq_elt = *freq_elt * (1 + (w->alpha)*w_d);
    return;
   case POWR_SCHEMA:
    printf("UNIMPLEMENTED!!!");
    return;
   default:
    return;
  }/*}}}*/
}

void fh_process_signal(complex *freq_buffer){
/*{{{*/
  char *wmark = w->w_message;
  int next_r = 0;
  for(int i = 0; i < WMARK_LEN; i++){
    if(wmark[i] == '\0')
      break;
    next_r = next_rand(i);
    //printf("%f ", freq_buffer[next_r]);
    //if(embed_debug)
     // printf("%5.d ", next_r);
    watermark_elt(c_to_d(wmark[i]), &(freq_buffer[next_r]));
  }
  //if(embed_debug)
  //  putchar('\n');

  /*
   * Gets stats of fourier domain real values
   *
  double stats[WMARK_LEN];
  double mean, min, max;
  for(int i = 0; i < WMARK_LEN; i++)
    //stats[i] = creal(freq_buffer[i]);
    stats[i] = (double)i;

  for(int i = 0; i < WMARK_LEN; i++){
    if(creal(freq_buffer[i]) > max) max = creal(freq_buffer[i]);
    if(creal(freq_buffer[i]) < min) min = creal(freq_buffer[i]);
    mean += creal(freq_buffer[i]) / WMARK_LEN;
  }
  //printf("mean = %f, max = %f, min = %f\n", mean, max, min);
  */
/*}}}*/
}

void ss_process_signal(complex *freq_buffer){
/*{{{*/
  int		f_buffer_len = BUFFER_LEN / 2 + 1;
  double	noise_seq[f_buffer_len];

  generate_noise(noise_seq, f_buffer_len);
  embed_to_noise(w->w_message, noise_seq, f_buffer_len);

  for(int i = 0; i < f_buffer_len; i++)
    watermark_elt(noise_seq[i], &(freq_buffer[i]));

/*}}}*/
}

int embed(char *infile_path, char *outfile_path, char *orig_outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;
  SNDFILE	*s_orig_out;

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

  // s_orig_out = watermaked audio file
  if(!(s_orig_out = sf_open(orig_outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"#######\terror opening the following file for writing: %s\n", orig_outfile_path);
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
    //embed_debug = (counter <= 3) ? 1 : 0;
    counter++;
    //printf("counter = %d\n", counter);
    if(embed_debug && counter == 3) printf("%d embed: \n",counter);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < BUFFER_LEN; i++){
	bytes_avg += time_buffer[i] / BUFFER_LEN;
      }
    }

    // write out the original data to test for differences.
    sf_write_double(s_orig_out, time_buffer, bytes_read);

    // only embed in a frame where we read in the full amount for the fourier transform
    if(bytes_read == BUFFER_LEN){
      fftw_execute(ft_forward);

      if(counter == 3 && embed_debug){
	printf("org: ");
	print_pow_density(freq_buffer, 10);
      }
      
      // embed the watermark into the frequency domain
      if(w->type == FH_EMBED)
	fh_process_signal(freq_buffer);
      else
	ss_process_signal(freq_buffer);

      if(counter == 3 && embed_debug){
	printf("new: ");
	print_pow_density(freq_buffer, 10);
      }

      fftw_execute(ft_reverse);

      // The DFT naturally multiplies every array element by the size of the array, reverse this
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);

    } // if(bytes_read == BUFFER_LEN)

    sf_write_double(s_outfile, time_buffer, bytes_read);

    if(counter == 3 && embed_debug){
      fftw_execute(ft_forward);
      printf("aft: ");
      print_pow_density(freq_buffer, 10);
      fftw_execute(ft_reverse);
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);
    }

  } //while

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
  sf_close(s_orig_out);
/*}}}*/
} //embed

void print_embedding_info(char *infile_path, char *outfile_path, char *orig_out_path, char *config_path)
{/*{{{*/
  char buffer[1000];
  int l_to_wmark = strlen(infile_path) + strlen("transform") + 6;

  puts("");
  int i;
  // get line 1
  for(i = 0; i < l_to_wmark; i++)
    buffer[i] = ' ';
  strncpy(buffer + i, config_path, strlen(config_path));
  i += strlen(config_path);
  buffer[i] = '\0';
  puts(buffer);

  // line 2
  for(i = 0; i < l_to_wmark + 2; i++)
    buffer[i] = ' ';
  buffer[i] = '|';
  buffer[i+1] = '\0';
  puts(buffer);

  // line 3
  for(i = 0; i < l_to_wmark + 2; i++)
    buffer[i] = ' ';
  buffer[i] = 'v';
  buffer[i+1] = '\0';
  puts(buffer);

  // line 4
  strncpy(buffer, infile_path, strlen(infile_path));
  i = strlen(infile_path);

  strncpy(buffer + i, "-->", 3);
  i += 3;

  strncpy(buffer+i, "transform", strlen("transform"));
  i += strlen("transform");

  strncpy(buffer + i, "-->", 3);
  i += 3;
  
  strncpy(buffer + i, "embed", strlen("embed"));
  i += strlen("embed");

  strncpy(buffer + i, "-->", 3);
  i += 3;

  strncpy(buffer+i, "transform", strlen("transform"));
  i += strlen("transform");

  strncpy(buffer + i, "-->", 3);
  i += 3;

  strncpy(buffer + i, outfile_path, strlen(outfile_path)); 
  buffer[i + strlen(outfile_path)] = '\0';

  puts(buffer);

  // line 5
  for(i = 0; i < strlen(infile_path) + 8; i++)
    buffer[i] = ' ';
  buffer[i++] = '|';
  buffer[i] = '\0';
  puts(buffer);
  
  // line 6
  for(i = 0; i < strlen(infile_path) + 8; i++)
    buffer[i] = ' ';
  buffer[i++] = 'L';
  for(; i < l_to_wmark + strlen("embed") + 2; i++)
    buffer[i] = '-';
  buffer[i++] = '>';

  strncpy(buffer+i, "transform", strlen("transform"));
  i += strlen("transform");

  strncpy(buffer + i, "-->", 3);
  i += 3;

  strncpy(buffer + i, orig_out_path, strlen(orig_out_path));
  buffer[i + strlen(orig_out_path)] = '\0';

  puts(buffer);
  puts("");
}/*}}}*/
