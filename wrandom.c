#include <stdio.h>
#include <stdlib.h>
#include "wrandom.h"

int *my_random = 0;
int random_size = 0;
unsigned int random_seed = 0;

unsigned seed_rand(unsigned seed){
  srand(seed);
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
