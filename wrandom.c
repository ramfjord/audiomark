#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wrandom.h"
#define LARGE_DOUBLE 9999999.99

int *my_random = 0;
int random_size = 0;
unsigned int random_seed = 0;

// seed all random number generators - TODO change so require a long?
unsigned seed_rand(unsigned seed){
  srand(seed);
	srand48((long)seed);
  return seed;
}

void free_rand(){
  free(my_random);
}

// sets an array of numbers to be randomized
void set_rand(int size){
  if(my_random)
    free_rand();
  random_size = size;
  my_random = (int *)malloc(sizeof(int) * random_size);
  for(int i = 0; i < random_size; i++)
    my_random[i] = i;
}

// returns a random integer between 0 and random_size, that hasn't been chosen
// in the past n_chosen calls.
int next_rand(int n_chosen){
  if(random_size - n_chosen <= 0){
    fprintf(stderr,"n_chosen (%d) is greater than random_size (%d)!!\n",n_chosen, random_size);
    return -1;
  }
  int i = rand() % (random_size - n_chosen);

  int temp = my_random[i];
  my_random[i] = my_random[random_size - n_chosen - 1];
  my_random[random_size - n_chosen - 1] = temp;

  return temp;
}

// generates a random double from the uniform distribution [0,1)
double rand_double(){
  return drand48();
}

// generate a random double from the normal distribution with mean 0, var 1
// uses the Box-Muller transform, which produces 2 random numbers at a time.
double norm_double(){
	static double Z2;
	static int Z2_def = 0;

	if(Z2_def){
		Z2_def = 0;
		return Z2;
	}

	// generate two new independent normally distributed numbers, Z1 and Z2, from
	// uniformly distributed numbers U1 and U2.  U1 and U2 must be from the
	// uniform distribution (0,1] for Z1 and Z2 to be from the standard
	// distribution with mean 0 varience 1.
	double U1 = rand_double();
	double U2 = rand_double();
	if(U1 == 0) U1 = 1;
	if(U2 == 0) U2 = 1;

	Z2_def = 1;
	Z2 = sqrt(-2 * log(U1))*sin(2 * M_PI * U2);
	// below would be Z1
	return sqrt(-2 * log(U1))*cos(2 * M_PI * U2);
}

// knuth-fischer-yates shuffle
int shuffle_d_arr(double *buffer, int n){
	for(int i = n-1; i >= 0; i--){
		int j = rand() % i;
		double tmp = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = tmp;
	}
}
