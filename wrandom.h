/* Function headers for wrandom methods
 * 
 * @author Thomas Ramfjord
*/

#ifndef WRANDOM
#define WRANDOM 1

unsigned seed_rand(unsigned seed);
void free_rand();

// sets an array of numbers to be randomized
void set_rand(int size);

// returns a random integer between 0 and random_size, that hasn't been chosen
// in the past n_chosen calls.
int next_rand(int n_chosen);

// returns a random double from the uniform distribution [0,1)
double rand_double();

// returns a random double from the normal distribution N(0,1)
double norm_double();


int shuffle_d_arr(double *buffer, int n);
#endif
