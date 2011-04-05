#ifndef WATERMARK_H
#define WATERMARK_H 1

#define BUFFER_LEN 1024
#define WMARK_LEN (BUFFER_LEN/2 + 1)
#define PATH_SIZE 64

#define ERROR (-1)
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

int	parse_config(char *config);
void  	print_watermark_info();
void  	print_sfile_info(SF_INFO sfinfo);
double	c_to_d(char a);
void	generate_noise(double *buffer, int buffer_len);
void  	embed_to_noise(char *wmark, double *noise_seq, int buffer_len);

int	get_schema(char *s);
int   	set_schema(int s);
int   	set_alpha(int a);
#endif
