#ifndef WATERMARK
#define WATERMARK 1

#define BUFFER_LEN 1024
#define WMARK_LEN (BUFFER_LEN/2 + 1)
#define PATH_SIZE 64
#define PLUS_SCHEMA 0
#define MULT_SCHEMA 1
#define POWR_SCHEMA 2
#define NOISE_FACTOR 1

#define SS_EMBED 1
#define FH_EMBED 2

typedef struct watermark{
  char *w_message;
  int w_len;
  int schema;
  double alpha;
  unsigned key_seed;
  int processing_gain;
  int type;
} watermark;

void parse_config(char *config);
void print_d_array(double *array, int n);
void print_r_complex_array(complex *array, int n);
void print_watermark_info();
void print_sfile_info(SF_INFO sfinfo);
double c_to_d(char a);
void array_div(int n, double *array, int size);
void complex_array_div(int n, complex *array, int size);
void array_add(double *array, double *addition, int size);
void complex_array_add(complex *a, complex *addition, int len);
void d_array_to_complex(complex *a, double *addition, int len);
double rand_double();
void generate_noise(double *buffer, int buffer_len);
void embed_to_noise(char *wmark, double *noise_seq, int buffer_len);

int w_embed(char *infile_path, char *outfile_path, char *orig_outfile_path);
int fh_embed(char *infile_path, char *outfile_path);
int ss_embed(char *infile_path, char *outfile_path);

int test(char *orig_path, char *test_path);
int fh_test(char *orig_path, char *test_path);
int ss_test(char *orig_path, char *test_path);

int get_schema(char *s);
int set_schema(int s);
int set_alpha(int a);

complex to_real(char w_c);
double extract_wmark_elt(complex orig, complex test);
void watermark_elt(double w_d, complex *freq_elt);

//extracts the watermark from the test_freq_buffer, given the orig_freq_buffer, and the length of the watermark
void extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer);

void process_signal(complex *freq_buffer);

int test(char *orig_path, char *test_path);

//void get_input(char *out_array, int max_len, char *prompt);
#endif
