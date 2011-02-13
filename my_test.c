#include <complex.h>
#include <fftw3.h>
#include <sndfile.h>
#include <stdlib.h>
#include <stdio.h>

void array_div(int n, double *array, int size){
  for(int i = 0; i < size; i++)
    array[i] /= n;
}
void complex_array_div(int n, complex *array, int size){
  for(int i = 0; i < size; i++)
    array[i] /= n;
}
int main(int argc, char *argv[]){
  printf("argc = %d, argv[1] = %s, argv[2] = %s\n",argc, argv[1], argv[2]);
  char path[] = "/home/thomas/Workspace/test.wav";

  SF_INFO	sfinfo;
  SNDFILE   	*s_file;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;
  
  complex x = 3+2*I;
  x *= 3;
  printf("x = %f + %fi\n", creal(x), cimag(x));

  // The pre-open SF_INFO should have format = 0;
  sfinfo.format = 0;
  
  //s_file = the audio file
  if(!(s_file = sf_open(path, SFM_READ, &sfinfo)))
    fprintf(stderr,"error opening the following file for reading: %s",path);

  printf("samplerate: %d\n", sfinfo.samplerate);
  printf("channels: %d\n", sfinfo.channels);
  printf("sections: %d\n", sfinfo.sections);
  //printf("samplerate: %d\n", sfinfo.samplerate);

  printf("sizeof(complex) = %d\n", sizeof(complex));
  printf("sizeof(double complex) = %d\n", sizeof(double complex));
  printf("sizeof(float complex) = %d\n", sizeof(float complex));
  printf("sizeof(long double complex) = %d\n", sizeof(long double complex));

  int n = 4;
  time_buffer = (double *) fftw_malloc(sizeof(double) * n);
  freq_buffer = (complex *) fftw_malloc(sizeof(double) * n);
  ft_forward = fftw_plan_dft_r2c_1d(n, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(n, freq_buffer, time_buffer, FFTW_ESTIMATE);
  
  time_buffer[0] = 0;
  time_buffer[1] = 1;
  time_buffer[2] = 1;
  time_buffer[3] = 1;
  
  for(int i = 0; i < n; i++)
    printf("time_buffer[%d] = %f\n", i, time_buffer[i]);

  fftw_execute(ft_forward);

  //complex_array_div(n,freq_buffer,n);
  for(int i = 0; i < n; i++)
    printf("freq_buffer[%d] = [%f, %f i]\n", i, creal(freq_buffer[i]), cimag(freq_buffer[i]));

  fftw_execute(ft_reverse);

  array_div(n,time_buffer,n);
  for(int i = 0; i < n; i++)
    printf("time_buffer[%d] = %f\n", i, time_buffer[i]);

  //free everything
  //fftw_free(time_buffer);
  //fftw_free(freq_buffer);
  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  sf_close(s_file);
}

