#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w_array_ops.h"

void print_r_complex_array(complex *array, int n){
  printf("[%8.4lf",creal(array[0]));
  for(int i = 1; i < n; i++) 
    printf(", %8.4lf",creal(array[i]));
  printf("]\n");
}

double pow_elem(complex elem){
  return sqrt(pow(creal(elem),2) + pow(cimag(elem), 2));
  // TODO change back
  //return creal(elem);
}

void print_pow_density(complex *array, int n){
  printf("[%8.4lf", pow_elem(array[0]));
  for(int i = 1; i < n; i++) 
    printf(", %8.4lf",pow_elem(array[i]));
  printf("]\n");
}

void print_d_array(double *array, int n){
  printf("[%8.4lf",array[0]);
  for(int i = 1; i < n; i++) 
    printf(", %5.3lf",array[i]);
  printf("]\n");
}

// divide double *array of size size by n
void array_div(int n, double *array, int size){
/*{{{*/
  for(int i = 0; i < size; i++)
    array[i] /= n;
/*}}}*/
}

void array_add(double *array, double *addition, int size){
/*{{{*/
  for(int i = 0; i < size; i++)
    array[i] += addition[i];
/*}}}*/
}

void complex_array_div(int n, complex *array, int size){
/*{{{*/
  for(int i = 0; i < size; i++)
    array[i] /= n;
/*}}}*/
}

void complex_array_add(complex *a, complex *addition, int len){
  for(int i = 0; i < len; i++)
    a[i] += addition[i];
}

void d_array_to_complex(complex *a, double *addition, int len){
  for(int i = 0; i < len; i++)
    a[i] += addition[i];
}

void dub_x_dub_e_dub(double *a, double *b, double *c, int len){
	for(int i = 0; i < len; i++)
		c[i] = a[i] * b[i];
}

// helper for get_n_biggest: returns the index of indices such that
// buffer[indices[i]] is minimized
int get_min_from_indices(int *indices, complex *buffer, int n)
{ //{{{
	if(indices[0] == -1) return 0;

	int min = 0;
	int min_index = 0;
	double min_pow = pow_elem(buffer[indices[0]]);
	for(int i = 1; i < n; i++){
		if(indices[i] == -1)
			return i;
		if(min_pow > pow_elem(buffer[indices[i]])){
			min_pow = pow_elem(buffer[indices[i]]);
			min = indices[i];
			min_index = i;
		}
	}
	return min_index;
} //}}}

//
// Returns an array of the indices of the n largest components in a complex
// array of syze n.
// - the size of an elem is determined by pow_elem
// - order n*len time - could probably be faster
//
int *get_n_biggest(complex *buffer, int len, int n)
{ //{{{
	int *indices = (int *) malloc(n * sizeof(int));
	for(int i = 0; i < n; i++) indices[i] = -1;

	double min_pow = -1;
	int min_index=0;
	for(int i = 0; i < len; i++){
		if(pow_elem(buffer[i]) > min_pow){
			indices[min_index] = i;
			min_index = get_min_from_indices(indices, buffer, n);
			min_pow = (indices[min_index] == -1) ? -1 : 
								pow_elem(buffer[indices[min_index]]);
		}
	}
	return indices;
} //}}}
