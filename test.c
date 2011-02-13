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

  parse_config(config_path);

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

  free(w->w_message);
  //free(w);
} // main
