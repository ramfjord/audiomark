void print_d_array(double *array, int n);
void print_r_complex_array(complex *array, int n);

void array_div(int n, double *array, int size);
void complex_array_div(int n, complex *array, int size);
void array_add(double *array, double *addition, int size);
void complex_array_add(complex *a, complex *addition, int len);
void d_array_to_complex(complex *a, double *addition, int len);
double pow_elem(complex elem);
void print_pow_density(complex *array, int n);
int *get_n_biggest(complex *buffer, int len, int n);
