#ifndef W_ARRAY_OPS_H
#define W_ARRAY_OPS_H 1

#define A_FREQ_LEN 513

void _print_int(void *elt);
void _print_double(void *elt);
void _print_double_stdout(void *elt);
void _print_pow_dens(void *elt);
void _print_complex(void *elt);
void print_i_array(int *array, int n);
void print_d_array(double *array, int n);
void print_complex_array(complex *array, int n);
void print_r_complex_array(complex *array, int n);
void print_array(char *array, int n, int eltsize, void (*print)(void *));

void array_div(int n, double *array, int size);
void swap_d(double *a, double *b);
void complex_array_div(int n, complex *array, int size);
void array_add(double *array, double *addition, int size);
void complex_array_add(complex *a, complex *addition, int len);
void d_array_to_complex(complex *a, double *addition, int len);
double pow_elem(complex elem);
void print_pow_density(complex *array, int n);
int *get_n_biggest(complex *buffer, int len, int n);

#endif
