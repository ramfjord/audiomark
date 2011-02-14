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

extern watermark *w;

void print_embedding_info(char *infile_path, char *outfile_path, char *orig_out_path, char *config_path);

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

  w_embed(infile_path, outfile_path, orig_out_path);

  print_watermark_info();

freedom:
  free(w->w_message);
  free(w);
} // main

void print_embedding_info(char *infile_path, char *outfile_path, char *orig_out_path, char *config_path){
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
}
