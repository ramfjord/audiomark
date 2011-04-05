#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <sndfile.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "wrandom.h"
#include "watermark.h"
#include "w_array_ops.h"

int		watermark_debug = 1;
watermark	*w;

//
// Parse config file to fill out w
//
int parse_config(char *config_path){
/*{{{*/
  FILE *config;
  if(config = fopen(config_path, "r")){
    char read_buffer[BUFFER_LEN];
    while(1){
      if(fgets(read_buffer, BUFFER_LEN, config) == NULL)
	break;

      if(strncmp(read_buffer, "watermark ", strlen("watermark ")) == 0){
	w->w_message = (char *)malloc(strlen(read_buffer) - strlen("watermark "));
	memset(w->w_message, 0, strlen(read_buffer) - strlen("watermark "));

	char *w_c = w->w_message;
	char *tmp = read_buffer + strlen("watermark ");
	char delim = *tmp;
	tmp++;

	int i;
	for(i = 0; i < WMARK_LEN && tmp[i] != delim; i++)
	  w_c[i] = tmp[i];
	w->w_len = strlen(w->w_message);
      }

      else if(strncmp(read_buffer, "alpha ", strlen("alpha ")) == 0){
	if(EOF == sscanf(read_buffer, "alpha %lf", &(w->alpha)))
	  return ERROR;
      }

      else if(strncmp(read_buffer, "schema ", strlen("schema ")) == 0){
	if(ERROR == (w->schema = get_schema((read_buffer + strlen("schema ")))))
	  return ERROR;
      }

      else if(strncmp(read_buffer, "seed ", strlen("seed ")) == 0){
	if(EOF == sscanf(read_buffer, "seed %d", &(w->key_seed)))
	  return ERROR;
      }
      else if(strncmp(read_buffer, "processing_gain ", strlen("processing_gain ")) == 0){
	if(EOF == sscanf(read_buffer, "processing_gain %d", &(w->processing_gain)))
	  return ERROR;
      }

      else if(strncmp(read_buffer, "type ", strlen("type ")) == 0){
	char *tmp = read_buffer + strlen("type ");
	if(strncmp(tmp, "fh", 2) == 0)
	  w->type = FH_EMBED;
	else
	  w->type = SS_EMBED;
      }
    }
  }
/*}}}*/
}

//
// Get's schema info from a string
//
int get_schema(char *s){
/*{{{*/
  if(strstr(s, "plus"))
    w->schema = PLUS_SCHEMA;
  else if(strstr(s, "mult"))
    w->schema = MULT_SCHEMA;
  else if(strstr(s, "powr"))
    w->schema = POWR_SCHEMA;
  else{
    fprintf(stderr, "Invalid Schema: the valid schemas are plus, mult, and powr. \n");
    fprintf(stderr, "plus: v'_i = v_i+aw_i\n");
    fprintf(stderr, "mult: v'_i = v_i(1+aw_i)\n");
    fprintf(stderr, "powr: v'_i = v_i(e*a^w_i)\n");
    return ERROR;
  }
  return w->schema;
/*}}}*/
}
      
int set_schema(int s){
/*{{{*/
  switch (s) {
    case PLUS_SCHEMA: w->schema = PLUS_SCHEMA; return PLUS_SCHEMA;
    case MULT_SCHEMA: w->schema = MULT_SCHEMA; return MULT_SCHEMA;
    case POWR_SCHEMA: w->schema = POWR_SCHEMA; return POWR_SCHEMA;
    default:
      printf("The valid schemas are plus, mult, and powr. \n");
      printf("plus: v'_i = v_i+aw_i\n");
      printf("mult: v'_i = v_i(1+aw_i)\n");
      printf("powr: v'_i = v_i(e*a^w_i)\n");
  }
  return -1;
/*}}}*/
}

int set_alpha(int a){
  w->alpha = a;
  return a;
}

void print_watermark_info(){
/*{{{*/
  printf("watermark info:\n");
  printf("\twatermark: >%s<\n",w->w_message);
  printf("\tlen (real %d), %d\n", strlen(w->w_message), w->w_len);
  printf("\talpha: %lf\n",w->alpha);
  printf("\tschema: %d\n",w->schema);
  printf("\tkey_seed: %u\n",w->key_seed);
  printf("\tprocessing_gain: %d\n", w->processing_gain);
  printf("\ttype = %s\n", (w->type == FH_EMBED) ? "fh" : "ss");
/*}}}*/
}

void print_sfile_info(SF_INFO sfinfo){
/*{{{*/
  printf("sound file info:\n");
  printf("\tsamplerate: %d\n", sfinfo.samplerate);
  printf("\tchannels: %d\n", sfinfo.channels);
  printf("\tsections: %d\n", sfinfo.sections);
  printf("\tformat: %d\n", sfinfo.format);
  printf("\tframes: %d\n", (int)sfinfo.frames);
  printf("\tseekable: %d\n", sfinfo.seekable);
/*}}}*/
}

double c_to_d(char a){
/*{{{*/
  int tmp = a % 10;
  return (tmp > 5) ? (double)(tmp - 5) : (double)(tmp - 6);
/*}}}*/
}

//
// Generates an array of noise in buffer
// this is a random sequence of positive/negative elements of magnitude NOISE_FACTOR
//
void generate_noise(double *buffer, int buffer_len){
/*{{{*/
  for(int i = 0; i < buffer_len; i++){
    int sign = rand()%2;
    if(sign == 0) sign = -1;
    buffer[i] = NOISE_FACTOR * (double)sign;
  }
/*}}}*/
}

// 
// Goes through each bit in wmark
// flips bits in noise_seq if the current watermark bit is 0
//
void embed_to_noise(char *wmark, double *noise_seq, int buffer_len){
/*{{{*/
  int seg_len = w->processing_gain;
  for(int i = 0; i < w->w_len * 8; i++){ //iterate through the bits in wmark
    unsigned char cur_char = wmark[i/8];
    int cur_bit;
    if(cur_char & (1 << (i % 8)))
      cur_bit = 1;
    else
      cur_bit = -1;

    for(int j = 0; j < seg_len; j++) // use each bit to change processing_gain elts
      if(i*seg_len + j < BUFFER_LEN/2 + 1)
	noise_seq[i*seg_len + j] *= cur_bit;
  }
/*}}}*/
}

// Reads input from console, and stores it in out_array.
//
// stops on 2 newlines in a row (in which case none are added)
// stops when max_len characters are read.
//void get_input(char *out_array, int max_len, char *prompt){
///*{{{*/
//  char *buffer;
//  int cur_len = 0;
//  int num_nlines = 0;
//
//  while(1){
//    buffer = readline(prompt);
//    int b_len = strlen(buffer);
//
//    if(b_len < 1) break;
//
//    if(b_len >= max_len - cur_len){
//      strncpy(out_array, buffer, (max_len - cur_len - 1)); //reserve the last char for the \0
//      break;
//    }
//
//    strncpy(out_array,buffer,b_len);
//  }
//
///*}}}*/
//}
