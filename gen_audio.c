#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <sndfile.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PATH_SIZE 100
#define NUM_SECS 2
#define CD_SAMPLERATE 44100

void print_d_array(double *array, int n);
void print_complex_array(complex *array, int n);
void array_div(int n, double *array, int size);

int main(int argc, char *argv[]){
  char		infile_path[PATH_SIZE];
  char		outfile1_path[PATH_SIZE];
  char		outfile2_path[PATH_SIZE];
  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile1;
  SNDFILE   	*s_outfile2;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*orig_buffer;
  double	*time_buffer;
  complex	*freq_buffer;

  int		buffer_len;
  double	note_hz = 440;
  double	note2_hz = 500;

  strcpy(infile_path, "input.wav");
  strcpy(outfile1_path, "out1.wav");
  strcpy(outfile2_path, "out2.wav");

  if(argc > 1)
    note_hz = strtod(argv[1], NULL);
  if(argc > 2)
    note2_hz = strtod(argv[2], NULL);
  
  //if(argc > 2)
  //  strncpy(infile_path, argv[2], PATH_SIZE);
  //if(argc > 3)
  //  strncpy(outfile1_path, argv[3], PATH_SIZE);

  // The pre-open SF_INFO should have format = 0, everything else will be set in the open call
  sfinfo.format = 0;
  
  // s_infile = input audio file
  if(!(s_infile = sf_open(infile_path, SFM_READ, &sfinfo))){
    fprintf(stderr,"error opening the following file for reading: %s\n", infile_path);
    return 1;
  }
  sfinfo.samplerate = CD_SAMPLERATE;

  // s_outfile = watermaked audio file
  if(!(s_outfile1 = sf_open(outfile1_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile1_path);
    return 1;
  }

  // s_outfile = watermaked audio file
  if(!(s_outfile2= sf_open(outfile2_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile2_path);
    return 1;
  }

  printf("samplerate: %d\n", sfinfo.samplerate);
  printf("channels: %d\n", sfinfo.channels);
  printf("sections: %d\n", sfinfo.sections);
  printf("format: %d\n", sfinfo.format);
  printf("frames: %d\n", (int)sfinfo.frames);
  printf("seekable: %d\n", sfinfo.seekable);

  buffer_len = sfinfo.samplerate;
  //buffer_len = sfinfo.samplerate * 2;

  time_buffer = (double *) malloc(sizeof(double) * buffer_len);
  orig_buffer = (double *) malloc(sizeof(double) * buffer_len);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (buffer_len/2 + 1));
  
  ft_forward = fftw_plan_dft_r2c_1d(buffer_len, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(buffer_len, freq_buffer, time_buffer, FFTW_ESTIMATE);

  double my_time = 0.0;
  double my_time2 = 0.0;
  for(int t = 0; t < NUM_SECS * (double)sfinfo.samplerate / (double)buffer_len; t++){
    for(int i = 0; i < buffer_len; i++){
      orig_buffer[i] = sin(my_time);
      //orig_buffer[i] += cos(my_time2);
      orig_buffer[i] /= 2;
      time_buffer[i] = orig_buffer[i];
      //my_time += (M_PI * note_hz / sfinfo.samplerate) * (1 + sin(M_PI * i / sfinfo.samplerate)); 
      my_time += M_PI * note_hz / sfinfo.samplerate;
      my_time2 += M_PI * note2_hz / sfinfo.samplerate;
    }
//    if(t == 0){ 
//      print_d_array(time_buffer, buffer_len);
//      puts("##########################\n\n");
//    }
//
    fftw_execute(ft_forward);
//    if(t == 0){ 
//      print_complex_array(freq_buffer, buffer_len/2 + 1);
//      puts("##########################\n\n");
//    }
    fftw_execute(ft_reverse);
//    if(t == 0){ 
//      print_d_array(time_buffer, buffer_len);
//    }
    array_div(buffer_len, time_buffer, buffer_len);


    sf_write_double(s_outfile1, orig_buffer, buffer_len);
    sf_write_double(s_outfile2, time_buffer, buffer_len);
  }

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  sf_close(s_infile);
  sf_close(s_outfile1);
  sf_close(s_outfile2);
  system("play out1.wav");
}

void print_d_array(double *array, int n){
  printf("[%8.4lf",array[0]);
  for(int i = 1; i < n; i++) {
    printf(", %5.3lf",array[i]);
    if(i%16 == 0) putchar('\n');
  }
  printf("]\n");
}

void print_complex_array(complex *array, int n){
  printf("[(%8.4lf, %8.4lf)",creal(array[0]), cimag(array[0]));
  for(int i = 1; i < n; i++) {
    printf(", (%8.4lf, %8.4lf)",creal(array[i]), cimag(array[i]));
    if(i%8 == 0) putchar('\n');
  }
  printf("]\n");
}

void array_div(int n, double *array, int size){
/*{{{*/
  for(int i = 0; i < size; i++)
    array[i] /= (double)n;
/*}}}*/
}
