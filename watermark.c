#include <complex.h>
#include <math.h>
#include <fftw3.h>
#include <sndfile.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "wrandom.h"
#include "watermark.h"

//int		wmark_schema;
//double		wmark_alpha;
int		debug = 1;
watermark	*w;

void parse_config(char *config_path){
/*{{{*/
  FILE *config;
  if(config = fopen(config_path, "r")){
    char read_buffer[BUFFER_LEN];
    while(1){
      if(fgets(read_buffer, BUFFER_LEN, config) == NULL)
	break;

      if(strncmp(read_buffer, "watermark ", strlen("watermark ")) == 0){
	w->w_message = (char *)malloc(strlen(read_buffer) - strlen("watermark "));
	char *w_c = w->w_message;
	char *tmp = read_buffer + strlen("watermark ");
	char delim = *tmp;
	tmp++;

	int i;
	for(i = 0; i < WMARK_LEN && tmp[i] != delim; i++)
	  w_c[i] = tmp[i];
	w->w_len = strlen(w->w_message);
      }

      else if(strncmp(read_buffer, "alpha ", strlen("alpha ")) == 0){
	sscanf(read_buffer, "alpha %lf", &(w->alpha));
      }

      else if(strncmp(read_buffer, "schema ", strlen("schema ")) == 0){
	w->schema = get_schema((read_buffer + strlen("schema ")));
      }

      else if(strncmp(read_buffer, "seed ", strlen("seed ")) == 0){
	sscanf(read_buffer, "seed %d", &(w->key_seed));
      }
      else if(strncmp(read_buffer, "processing_gain ", strlen("processing_gain ")) == 0){
	sscanf(read_buffer, "processing_gain %d", &(w->processing_gain));
      }

      else if(strncmp(read_buffer, "type ", strlen("type ")) == 0){
	char *tmp = read_buffer + strlen("type ");
	if(strncmp(tmp, "fh", 2) == 0)
	  w->type = FH_EMBED;
	else
	  w->type = SS_EMBED;
      }
    }
  }
/*}}}*/
}

void print_watermark_info(){
/*{{{*/
  printf("watermark: >%s<\n",w->w_message);
  printf("len (real %d), %d\n", strlen(w->w_message), w->w_len);
  printf("alpha: %lf\n",w->alpha);
  printf("schema: %d\n",w->schema);
  printf("key_seed: %u\n",w->key_seed);
  printf("processing_gain: %d\n", w->processing_gain);
  printf("type = %s\n", (w->type == FH_EMBED) ? "fh" : "ss");
/*}}}*/
}

void print_sfile_info(SF_INFO sfinfo){
/*{{{*/
  printf("samplerate: %d\n", sfinfo.samplerate);
  printf("channels: %d\n", sfinfo.channels);
  printf("sections: %d\n", sfinfo.sections);
  printf("format: %d\n", sfinfo.format);
  printf("frames: %d\n", (int)sfinfo.frames);
  printf("seekable: %d\n", sfinfo.seekable);
/*}}}*/
}

void print_r_complex_array(complex *array, int n){
  printf("[%8.4lf",creal(array[0]));
  for(int i = 1; i < n; i++) 
    printf(", %8.4lf",creal(array[i]));
  printf("]\n");
}

double pow_elem(complex elem){
  return sqrt(pow(creal(elem),2) + pow(cimag(elem), 2));
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

double c_to_d(char a){
/*{{{*/
  int tmp = a % 10;
  return (tmp > 5) ? (double)(tmp - 5) : (double)(tmp - 6);
/*}}}*/
}

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

double rand_double(){
  return (double)rand() / (double)RAND_MAX;
}

void generate_noise(double *buffer, int buffer_len){
/*{{{*/
  for(int i = 0; i < buffer_len; i++){
    int sign = rand()%2;
    if(sign == 0) sign = -1;
    buffer[i] = NOISE_FACTOR * (double)sign;
  }
/*}}}*/
}

void embed_to_noise(char *wmark, double *noise_seq, int buffer_len){
/*{{{*/
  int seg_len = w->processing_gain;
  for(int i = 0; i < w->w_len * 8; i++){ //iterate through the bits in wmark
    unsigned char cur_char = wmark[i/8];
    int cur_bit;
    if(cur_char & (1 << (i % 8)))
      cur_bit = 1;
    else
      cur_bit = -1;

    for(int j = 0; j < seg_len; j++) // use each bit to change processing_gain elts
      if(i*seg_len + j < BUFFER_LEN/2 + 1)
	noise_seq[i*seg_len + j] *= cur_bit;
  }
/*}}}*/
}
      

int get_schema(char *s){
/*{{{*/
  if(strstr(s, "plus"))
    w->schema = PLUS_SCHEMA;
  else if(strstr(s, "mult"))
    w->schema = MULT_SCHEMA;
  else if(strstr(s, "powr"))
    w->schema = POWR_SCHEMA;
  else{
    printf("The valid schemas are plus, mult, and powr. \n");
    printf("plus: v'_i = v_i+aw_i\n");
    printf("mult: v'_i = v_i(1+aw_i)\n");
    printf("powr: v'_i = v_i(e*a^w_i)\n");
  }
  return w->schema;
/*}}}*/
}

int set_schema(int s){
/*{{{*/
  switch (s) {
    case PLUS_SCHEMA: w->schema = PLUS_SCHEMA; return PLUS_SCHEMA;
    case MULT_SCHEMA: w->schema = MULT_SCHEMA; return MULT_SCHEMA;
    case POWR_SCHEMA: w->schema = POWR_SCHEMA; return POWR_SCHEMA;
    default:
      printf("The valid schemas are plus, mult, and powr. \n");
      printf("plus: v'_i = v_i+aw_i\n");
      printf("mult: v'_i = v_i(1+aw_i)\n");
      printf("powr: v'_i = v_i(e*a^w_i)\n");
  }
  return -1;
/*}}}*/
}

int set_alpha(int a){
  w->alpha = a;
  return a;
}

// Reads input from console, and stores it in out_array.
//
// stops on 2 newlines in a row (in which case none are added)
// stops when max_len characters are read.
//void get_input(char *out_array, int max_len, char *prompt){
///*{{{*/
//  char *buffer;
//  int cur_len = 0;
//  int num_nlines = 0;
//
//  while(1){
//    buffer = readline(prompt);
//    int b_len = strlen(buffer);
//
//    if(b_len < 1) break;
//
//    if(b_len >= max_len - cur_len){
//      strncpy(out_array, buffer, (max_len - cur_len - 1)); //reserve the last char for the \0
//      break;
//    }
//
//    strncpy(out_array,buffer,b_len);
//  }
//
///*}}}*/
//}

complex to_real(char w_c){
/*{{{*/
  complex ret = (w_c-'a') + 0I;
  return ret;
/*}}}*/
}

double extract_wmark_elt(complex orig, complex test){
/*{{{*/
  double orig_pow = sqrt(pow(creal(orig),2)+pow(cimag(orig),2));
  double test_pow = sqrt(pow(creal(test),2)+pow(cimag(test),2));

  switch (w->schema) {
   case PLUS_SCHEMA:
     return (test_pow - orig_pow) / w->alpha;
   case MULT_SCHEMA:
     return ((test_pow / orig_pow) - 1) / w->alpha;
   case POWR_SCHEMA:
    printf("UNIMPLEMENTED!!!");
    return 0;
   default:
    return 0;
  }
/*}}}*/
}

void watermark_elt(double w_d, complex *freq_elt){
/*{{{*/
  switch (w->schema) {
   case PLUS_SCHEMA:
    *freq_elt += w->alpha * w_d;
    *freq_elt += w->alpha * w_d * I;
    return;
   case MULT_SCHEMA:
    *freq_elt = *freq_elt * (1 + (w->alpha)*w_d);
    return;
   case POWR_SCHEMA:
    printf("UNIMPLEMENTED!!!");
    return;
   default:
    return;
  }/*}}}*/
}

//void fh_watermark_elt(char w_c, complex *freq_elt){
///*{{{*/
//  //printf("wmark_alph*c_to_d(watermar) = %f\n",w->alpha * c_to_d(w_c));
//  //printf("wmark_alph = %f\n",w->alpha);
//  //printf("c_to_d(watermar) = %f\n",c_to_d(w_c));
//  watermark_elt(c_to_d(w_c), freq_elt);
// switch (w->schema) {
//   case PLUS_SCHEMA:
//    *freq_elt += w->alpha * c_to_d(w_c);
//    return;
//   case MULT_SCHEMA:
//    *freq_elt = *freq_elt * (1 + (w->alpha)*c_to_d(w_c));
//    return;
//   case POWR_SCHEMA:
//    printf("UNIMPLEMENTED!!!");
//    return;
//   default:
//    return;
// }
//}/*}}}*/

//extracts the watermark from the test_freq_buffer, given the orig_freq_buffer, and the length of the watermark
void fh_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer){
/*{{{*/
  int cur_elt;
  for(int i = 0; i < w->w_len; i++){
    cur_elt = next_rand(i);
    if(debug)
      printf("%5.d ", cur_elt);
    extract_buffer[i] += extract_wmark_elt(orig_freq_buffer[cur_elt], test_freq_buffer[cur_elt]);
  }
  if(debug)
    putchar('\n');
/*}}}*/
}

void ss_extract_watermark(complex *orig_freq_buffer, complex *test_freq_buffer, double *extract_buffer, double *test_noise){
/*{{{*/
  int		f_buffer_len = BUFFER_LEN / 2 + 1;
  double	noise_seq[f_buffer_len];
  
  // testing to see if this improves performance:
  srand(w->key_seed);

  generate_noise(noise_seq, f_buffer_len);
  embed_to_noise(w->w_message, noise_seq, f_buffer_len);

  double tmp;
  for(int i = 0; i < f_buffer_len; i++){
    tmp = extract_wmark_elt(orig_freq_buffer[i], test_freq_buffer[i]);
    
    // to indicate a strong correlation, test_seq and extract_buffer should match
    // There is a strong correlation in this frame if noise_seq matches the extracted watermark elements
    //if(noise_seq[i] < 0) tmp *= -1;
    if(test_noise[i] < 0) tmp *= -1;
    extract_buffer[i] += tmp;
  }
/*}}}*/
}

void fh_process_signal(complex *freq_buffer){
/*{{{*/
  char *wmark = w->w_message;
  int next_r = 0;
  for(int i = 0; i < WMARK_LEN; i++){
    if(wmark[i] == '\0')
      break;
    next_r = next_rand(i);
    //printf("%f ", freq_buffer[next_r]);
    //if(debug)
     // printf("%5.d ", next_r);
    watermark_elt(c_to_d(wmark[i]), &(freq_buffer[next_r]));
  }
  //if(debug)
  //  putchar('\n');

  /*
   * Gets stats of fourier domain real values
   *
  double stats[WMARK_LEN];
  double mean, min, max;
  for(int i = 0; i < WMARK_LEN; i++)
    //stats[i] = creal(freq_buffer[i]);
    stats[i] = (double)i;

  for(int i = 0; i < WMARK_LEN; i++){
    if(creal(freq_buffer[i]) > max) max = creal(freq_buffer[i]);
    if(creal(freq_buffer[i]) < min) min = creal(freq_buffer[i]);
    mean += creal(freq_buffer[i]) / WMARK_LEN;
  }
  //printf("mean = %f, max = %f, min = %f\n", mean, max, min);
  */
/*}}}*/
}

void ss_process_signal(complex *freq_buffer){
/*{{{*/
  //int		f_buffer_len = w->processing_gain * w->w_len * 8;
  int		f_buffer_len = BUFFER_LEN / 2 + 1;
  double	noise_seq[f_buffer_len];
  
  // testing to see if this improves performance:
  srand(w->key_seed);

  generate_noise(noise_seq, f_buffer_len);
  embed_to_noise(w->w_message, noise_seq, f_buffer_len);

  for(int i = 0; i < f_buffer_len; i++)
    watermark_elt(noise_seq[i], &(freq_buffer[i]));

/*}}}*/
}

int w_embed(char *infile_path, char *outfile_path, char *orig_outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;
  SNDFILE	*s_orig_out;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;

  seed_rand(w->key_seed);
  set_rand(w->w_len); 

  //
  // Open input file and output file
  // -------------------------------

  // The pre-open SF_INFO should have format = 0, everything else will be set in the open call
  sfinfo.format = 0;
  
  // s_infile = input audio file
  if(!(s_infile = sf_open(infile_path, SFM_READ, &sfinfo))){
    fprintf(stderr,"error opening the following file for reading: %s\n", infile_path);
    return 1;
  }

  s_orig_out = sf_open(orig_outfile_path, SFM_WRITE, &sfinfo);
  // s_orig_out = watermaked audio file
  //if(!(s_orig_out = sf_open(orig_outfile_path, SFM_WRITE, &sfinfo))){
  //  fprintf(stderr,"#######\terror opening the following file for writing: %s\n", orig_outfile_path);
  //  return 1;
  //}

  // s_outfile = watermaked audio file
  if(!(s_outfile = sf_open(outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile_path);
    return 1;
  }

  print_sfile_info(sfinfo);

  //
  // Read, process, and write data
  // -----------------------------

  time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));
  
  ft_forward = fftw_plan_dft_r2c_1d(BUFFER_LEN, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(BUFFER_LEN, freq_buffer, time_buffer, FFTW_ESTIMATE);

  int bytes_read;

  int counter = 0;
  double bytes_avg;
  while(bytes_read = sf_read_double(s_infile, time_buffer, BUFFER_LEN)){
    //debug = (counter <= 3) ? 1 : 0;
    counter++;
    //printf("counter = %d\n", counter);
    if(debug && counter == 3) printf("%d embed: \n",counter);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < BUFFER_LEN; i++){
	bytes_avg += time_buffer[i] / BUFFER_LEN;
      }
    }

    // write out the original data to test for differences.
    sf_write_double(s_orig_out, time_buffer, bytes_read);

    // only embed in a frame where we read in the full amount for the fourier transform
    if(bytes_read == BUFFER_LEN){
      fftw_execute(ft_forward);

      if(counter == 3 && debug)
	print_pow_density(freq_buffer, 10);
      
      // embed the watermark into the frequency domain
      if(w->type == FH_EMBED)
	fh_process_signal(freq_buffer);
      else
	ss_process_signal(freq_buffer);

      if(counter == 3 && debug)
	print_pow_density(freq_buffer, 10);

      fftw_execute(ft_reverse);

      // The DFT naturally multiplies every array element by the size of the array, reverse this
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);

    } // if(bytes_read == BUFFER_LEN)

    sf_write_double(s_outfile, time_buffer, bytes_read);

    if(counter == 3 && debug){
      fftw_execute(ft_forward);
      print_pow_density(freq_buffer, 10);
      fftw_execute(ft_reverse);
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);
    }

  } //while

  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  free_rand();
  sf_close(s_infile);
  sf_close(s_outfile);
  sf_close(s_orig_out);
/*}}}*/
} //embed

int ss_embed(char *infile_path, char *outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  int		f_buffer_len = w->processing_gain * w->w_len * 8;
  int		t_buffer_len = (f_buffer_len-1) * 2;

  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;

  double	noise_seq[f_buffer_len];

  srand(w->key_seed);

  //
  // Open input file and output file
  // -------------------------------

  // The pre-open SF_INFO should have format = 0, everything else will be set in the open call
  sfinfo.format = 0;
  
  // s_infile = input audio file
  if(!(s_infile = sf_open(infile_path, SFM_READ, &sfinfo))){
    fprintf(stderr,"error opening the following file for reading: %s\n", infile_path);
    return 1;
  }
  printf("input file: %s\n",infile_path);

  // s_outfile = watermaked audio file
  if(!(s_outfile = sf_open(outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile_path);
    return 1;
  }
  printf("watermarked file: %s\n", outfile_path);

  printf("Input file info:\n");
  print_sfile_info(sfinfo);

  //
  // Read, process, and write data
  // -----------------------------

  time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);
  
  ft_forward = fftw_plan_dft_r2c_1d(t_buffer_len, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(t_buffer_len, freq_buffer, time_buffer, FFTW_ESTIMATE);

  int bytes_read;

  int counter = 0;
  double bytes_avg;
  while(bytes_read = sf_read_double(s_infile, time_buffer, t_buffer_len)){
    //debug = (counter <= 3) ? 1 : 0;
    counter++;

    if(debug) printf("%d embed: ",counter);

    if(bytes_read == t_buffer_len){
      if(counter == 2){
	printf("pre-watermark sample:\n");
	print_d_array(time_buffer, 10);
      }

      fftw_execute(ft_forward);

      if(counter == 2)
	print_r_complex_array(freq_buffer,10);

      generate_noise(noise_seq, f_buffer_len);
      embed_to_noise(wmark, noise_seq, f_buffer_len);
      //////complex_array_add(freq_buffer, noise_seq, f_buffer_len);
      d_array_to_complex(freq_buffer, noise_seq, f_buffer_len);
      ////print_d_array(noise_seq, f_buffer_len);

      if(counter == 2)
	print_r_complex_array(freq_buffer, 10);

      fftw_execute(ft_reverse);
      array_div(t_buffer_len, time_buffer, t_buffer_len);

      if(counter == 2){
      printf("post-watermark sample:\n");
      print_d_array(time_buffer, 10);
      }
    } //if

    sf_write_double(s_outfile, time_buffer, bytes_read);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < t_buffer_len; i++){
	bytes_avg += time_buffer[i] / t_buffer_len;
      }
      print_d_array(noise_seq, 10);
    }
  } //while

  //SNDFILE *sfile;
  //if(!(sfile = sf_open("test.wav", SFM_WRITE, &sfinfo))){
  //  fprintf(stderr,"error opening the following file for writing: %s\n", "test.wav");
  //  return 1;
  //}
  //printf("looped %d times, avg num 3 = %f\n", counter, bytes_avg);
  //for(int i = 0; i < t_buffer_len; i++)
  //  time_buffer[i] = 50;
  //for(int i = 0; i < counter; i++)
  //  sf_write_double(sfile, time_buffer, t_buffer_len);
  //sf_close(sfile);


  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  sf_close(s_infile);
  sf_close(s_outfile);
/*}}}*/
} //embed

int fh_embed(char *infile_path, char *outfile_path){
/*{{{*/
  char		*wmark = w->w_message;
  SF_INFO	sfinfo;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_infile;
  SNDFILE   	*s_outfile;

  fftw_plan 	ft_forward;
  fftw_plan 	ft_reverse;
  double	*time_buffer;
  complex	*freq_buffer;

  seed_rand(w->key_seed);
  set_rand(w->w_len); 

  //
  // Open input file and output file
  // -------------------------------

  // The pre-open SF_INFO should have format = 0, everything else will be set in the open call
  sfinfo.format = 0;
  
  // s_infile = input audio file
  if(!(s_infile = sf_open(infile_path, SFM_READ, &sfinfo))){
    fprintf(stderr,"error opening the following file for reading: %s\n", infile_path);
    return 1;
  }

  // s_outfile = watermaked audio file
  if(!(s_outfile = sf_open(outfile_path, SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", outfile_path);
    return 1;
  }

  print_sfile_info(sfinfo);

  //
  // Read, process, and write data
  // -----------------------------

  time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));
  
  ft_forward = fftw_plan_dft_r2c_1d(BUFFER_LEN, time_buffer, freq_buffer, FFTW_ESTIMATE);
  ft_reverse = fftw_plan_dft_c2r_1d(BUFFER_LEN, freq_buffer, time_buffer, FFTW_ESTIMATE);

  int bytes_read;

  int counter = 0;
  double bytes_avg;
  while(bytes_read = sf_read_double(s_infile, time_buffer, BUFFER_LEN)){
    debug = (counter <= 3) ? 1 : 0;
    counter++;
    if(debug) printf("%d embed: ",counter);
      
    if(counter == 3){
      bytes_avg = 0;
      for(int i = 0; i < BUFFER_LEN; i++){
	bytes_avg += time_buffer[i] / BUFFER_LEN;
      }
    }
    if(bytes_read == BUFFER_LEN){
      fftw_execute(ft_forward);
      //process_signal(freq_buffer);
      fftw_execute(ft_reverse);
      array_div(BUFFER_LEN, time_buffer, BUFFER_LEN);
    } //if

    sf_write_double(s_outfile, time_buffer, bytes_read);

  } //while

  SNDFILE *sfile;
  if(!(sfile = sf_open("test.wav", SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", "test.wav");
    return 1;
  }
  printf("looped %d times, avg num 3 = %f\n", counter, bytes_avg);
  for(int i = 0; i < BUFFER_LEN; i++)
    time_buffer[i] = 50;
  for(int i = 0; i < counter; i++)
    sf_write_double(sfile, time_buffer, BUFFER_LEN);
  sf_close(sfile);


  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_forward);
  fftw_destroy_plan(ft_reverse);
  free(time_buffer);
  fftw_free(freq_buffer);
  free_rand();
  sf_close(s_infile);
  sf_close(s_outfile);
/*}}}*/
} //embed


int fh_test(char *orig_path, char *test_path){
/*{{{*/
  char		*wmark  = w->w_message;
  SF_INFO	sfinfo_orig;		// struct with info on the samplerate, number of channels, etc
  SF_INFO	sfinfo_test;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_orig;
  SNDFILE   	*s_test;

  fftw_plan 	ft_orig;
  fftw_plan 	ft_test;
  double	*orig_time_buffer;
  complex	*orig_freq_buffer;

  double	*test_time_buffer;
  complex	*test_freq_buffer;

  double	*extract_buffer;

  // set up random number generator
  seed_rand(w->key_seed);
  set_rand(w->w_len); 

  sfinfo_orig.format = 0;
  if(!(s_orig = sf_open(orig_path, SFM_READ, &sfinfo_orig))){
    fprintf(stderr,"error opening the following file for reading: %s\n", orig_path);
    return 1;
  }

  char *t_path = test_path;
  /*while(t_path){
    if(*t_path = ' '){
      *t_path = '\0';
      break;
    }
  }
  */
  sfinfo_test.format = 0;
  if(!(s_test = sf_open(test_path, SFM_READ, &sfinfo_test))){
    fprintf(stderr,"error opening the following file for reading: >%s<\n", test_path);
    return 1;
  }


  orig_time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));

  test_time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));

  extract_buffer = (double *) malloc(sizeof(double) * (BUFFER_LEN/2 + 1));
  
  ft_orig = fftw_plan_dft_r2c_1d(BUFFER_LEN, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(BUFFER_LEN, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  int orig_bytes_read;
  int test_bytes_read;

  int counter = 0;
  while(1){
    debug = counter <= 3 ? 1 : 0;
    counter++;
    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, BUFFER_LEN);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, BUFFER_LEN);

    if(test_bytes_read != BUFFER_LEN || orig_bytes_read != BUFFER_LEN)
      break;

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    //extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer);
  }

  double orig_wmark[strlen(wmark)];
  double x_dot_x = 0, o_dot_x = 1;
  for(int i = 0; i < strlen(wmark); i++){
    orig_wmark[i] = c_to_d(wmark[i]);
    x_dot_x += extract_buffer[i] * extract_buffer[i];
    o_dot_x += orig_wmark[i] * extract_buffer[i];
  }

  puts("orig, then test");
  for(int i = 0; i < strlen(wmark); i++){
    printf("%5.2f\t",orig_wmark[i]);
  }
  putchar('\n');
  for(int i = 0; i < strlen(wmark); i++){
    printf("%5.2f\t",extract_buffer[i]);
  }
  putchar('\n');

  printf("sim(W,W*) = %f\n", o_dot_x / sqrt(x_dot_x));

  fftw_destroy_plan(ft_orig);
  fftw_destroy_plan(ft_test);
  fftw_free(orig_freq_buffer);
  fftw_free(test_freq_buffer);
  free(orig_time_buffer);
  free(test_time_buffer);
  free_rand();
/*}}}*/
}

int ss_test(char *orig_path, char *test_path){
/*{{{*/

  char		*wmark = w->w_message;
  int		f_buffer_len = w->processing_gain * w->w_len * 8;
  int		t_buffer_len = (f_buffer_len-1) * 2;

  SF_INFO	sfinfo_orig;		// struct with info on the samplerate, number of channels, etc
  SF_INFO	sfinfo_test;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_orig;
  SNDFILE   	*s_test;

  fftw_plan 	ft_orig;
  fftw_plan 	ft_test;
  double	*orig_time_buffer;
  complex	*orig_freq_buffer;

  double	*test_time_buffer;
  complex	*test_freq_buffer;

  double	noise_seq[f_buffer_len];
  //double	extract_buffer[w->w_len * 8];
  double	despread_buffer[f_buffer_len];

  srand(w->key_seed);

  sfinfo_orig.format = 0;
  if(!(s_orig = sf_open(orig_path, SFM_READ, &sfinfo_orig))){
    fprintf(stderr,"error opening the following file for reading: %s\n", orig_path);
    return 1;
  }
  printf("un-watermarked file: %s\n",orig_path);


  sfinfo_test.format = 0;
  if(!(s_test = sf_open(test_path, SFM_READ, &sfinfo_test))){
    fprintf(stderr,"error opening the following file for reading: >%s<\n", test_path);
    return 1;
  }
  printf("file to be tested: %s\n", test_path);


  orig_time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);

  test_time_buffer = (double *) malloc(sizeof(double) * t_buffer_len);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * f_buffer_len);

  //extract_buffer = (double *) malloc(sizeof(double) * f_buffer_len);
  
  ft_orig = fftw_plan_dft_r2c_1d(t_buffer_len, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(t_buffer_len, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  int orig_bytes_read;
  int test_bytes_read;

  int counter = 0;

  printf("unwatermarked file info:\n");
  printf("\tsamplerate: %d, %d\n", sfinfo_orig.samplerate, sfinfo_test.samplerate);
  printf("\tchannels: %d, %d\n", sfinfo_orig.channels, sfinfo_test.channels);
  printf("\tsections: %d, %d\n", sfinfo_orig.sections, sfinfo_test.sections);
  printf("\tformat: %d, %d\n", sfinfo_orig.format, sfinfo_test.format);
  printf("\tframes: %d, %d\n", (int)sfinfo_orig.frames, (int)sfinfo_test.frames);
  printf("\tseekable: %d, %d\n", sfinfo_orig.seekable, sfinfo_test.seekable);

  double bytes_avg;
  double o_dot_x;
  double x_dot_x;
  while(1){
    debug = (counter <= 3) ? 1 : 0;
    counter++;
    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, BUFFER_LEN);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, BUFFER_LEN);

    if(test_bytes_read != BUFFER_LEN || orig_bytes_read != BUFFER_LEN)
      break;

    if(counter == 2){
      printf("un-watermark sample:\n");
      print_d_array(orig_time_buffer, 10);
      printf("test sample:\n");
      print_d_array(test_time_buffer, 10);
    }

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    generate_noise(noise_seq, f_buffer_len);
    embed_to_noise(wmark, noise_seq, f_buffer_len);

    if(counter == 2){
      print_r_complex_array(orig_freq_buffer, 10);
      print_r_complex_array(test_freq_buffer, 10);
    }

    for(int i = 0; i < f_buffer_len; i++){
      test_freq_buffer[i] -= orig_freq_buffer[i];
      o_dot_x += creal(test_freq_buffer[i]) * noise_seq[i];
      x_dot_x += creal(test_freq_buffer[i]) * creal(test_freq_buffer[i]);
    }

    if(counter == 2){
      print_r_complex_array(test_freq_buffer, 10);
      print_d_array(noise_seq, 10);
    }
  } //while

  printf("sim(W,W*) = %f\n", o_dot_x / sqrt(x_dot_x));

  //
  // free everything
  // ---------------

  fftw_destroy_plan(ft_orig);
  fftw_destroy_plan(ft_test);
  fftw_free(orig_freq_buffer);
  fftw_free(test_freq_buffer);
  free(orig_time_buffer);
  free(test_time_buffer);
/*}}}*/
} 

int test(char *orig_path, char *test_path){
/*{{{*/
  char		*wmark  = w->w_message;
  SF_INFO	sfinfo_orig;		// struct with info on the samplerate, number of channels, etc
  SF_INFO	sfinfo_test;		// struct with info on the samplerate, number of channels, etc
  SNDFILE   	*s_orig;
  SNDFILE   	*s_test;

  fftw_plan 	ft_orig;
  fftw_plan 	ft_test;
  double	*orig_time_buffer;
  complex	*orig_freq_buffer;

  double	*test_time_buffer;
  complex	*test_freq_buffer;

  double	*extract_buffer;

  double	*orig_wmark;

  if(w->type == SS_EMBED){
    orig_wmark = (double *)malloc(sizeof(double) * (BUFFER_LEN/2 + 1));
    generate_noise(orig_wmark, BUFFER_LEN/2 + 1);
  }

  // set up random number generator
  seed_rand(w->key_seed);
  set_rand(w->w_len); 

  sfinfo_orig.format = 0;
  if(!(s_orig = sf_open(orig_path, SFM_READ, &sfinfo_orig))){
    fprintf(stderr,"error opening the following file for reading: %s\n", orig_path);
    return 1;
  }

  sfinfo_test.format = 0;
  if(!(s_test = sf_open(test_path, SFM_READ, &sfinfo_test))){
    fprintf(stderr,"error opening the following file for reading: >%s<\n", test_path);
    return 1;
  }


  orig_time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  orig_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));

  test_time_buffer = (double *) malloc(sizeof(double) * BUFFER_LEN);
  test_freq_buffer = (complex *) fftw_malloc(sizeof(complex) * (BUFFER_LEN/2 + 1));

  extract_buffer = (double *) malloc(sizeof(double) * (BUFFER_LEN/2 + 1));
  
  ft_orig = fftw_plan_dft_r2c_1d(BUFFER_LEN, orig_time_buffer, orig_freq_buffer, FFTW_ESTIMATE);
  ft_test = fftw_plan_dft_r2c_1d(BUFFER_LEN, test_time_buffer, test_freq_buffer, FFTW_ESTIMATE);

  int orig_bytes_read;
  int test_bytes_read;

  int counter = 0;
  while(1){
    debug = counter <= 3 ? 1 : 0;
    counter++;
    orig_bytes_read = sf_read_double(s_orig, orig_time_buffer, BUFFER_LEN);
    test_bytes_read = sf_read_double(s_test, test_time_buffer, BUFFER_LEN);

    if(test_bytes_read != BUFFER_LEN || orig_bytes_read != BUFFER_LEN)
      break;

    fftw_execute(ft_orig);
    fftw_execute(ft_test);

    if(counter == 3 && debug){
      print_pow_density(orig_freq_buffer, 10);
      print_pow_density(test_freq_buffer, 10);
    }
    if(w->type == FH_EMBED)
      fh_extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer);
    else
      ss_extract_watermark(orig_freq_buffer, test_freq_buffer, extract_buffer, orig_wmark);
  }

  array_div(counter, extract_buffer, BUFFER_LEN);

  double x_dot_x = 0, o_dot_x = 1;
  if(w->type == FH_EMBED){
  orig_wmark = malloc(sizeof(double) * strlen(wmark));
    for(int i = 0; i < strlen(wmark); i++){
      orig_wmark[i] = c_to_d(wmark[i]);
      x_dot_x += extract_buffer[i] * extract_buffer[i];
      o_dot_x += orig_wmark[i] * extract_buffer[i];
    }
  }
  else {
    double max_diff = 0;
    int max_i = 0;
    for(int i = 0; i < BUFFER_LEN/2+1; i++){
      x_dot_x += extract_buffer[i] * extract_buffer[i];
      o_dot_x += orig_wmark[i] * extract_buffer[i];
      if(extract_buffer[i] - orig_wmark[i] > max_diff){
	max_diff = extract_buffer[i] - orig_wmark[i];
	max_i = i;
      }
      if(orig_wmark[i] - extract_buffer[i] > max_diff){
	max_diff = orig_wmark[i] - extract_buffer[i];
	max_i = i;
      }
    }
    printf("max_diff = %f, i = %d\n", max_diff, max_i);
    for(int i = max_i - 5; i < max_i + w->processing_gain; i++)
      if(i > BUFFER_LEN/2+1) break;
      else
	printf("%5.2f\t", orig_wmark[i]);
    putchar('\n');
    for(int i = max_i - 5; i < max_i + w->processing_gain; i++)
      if(i > BUFFER_LEN/2+1) break;
      else
	printf("%5.2f\t", extract_buffer[i]);
  }

  puts("\norig, then test");
  int num_to_print;
  if(w->type == FH_EMBED)
    num_to_print = w->w_len;
  else
    num_to_print = 20;
  for(int i = 0; i < num_to_print; i++){
    printf("%5.2f\t",orig_wmark[i]);
  }
  putchar('\n');
  for(int i = 0; i < num_to_print; i++){
    printf("%5.2f\t",extract_buffer[i]);
  }
  putchar('\n');

  printf("W*.W* = %f, W.W* = %f\n", x_dot_x, o_dot_x);
  printf("sim(W,W*) = %f\n", o_dot_x / sqrt(x_dot_x));

  fftw_destroy_plan(ft_test);
  fftw_destroy_plan(ft_orig);
  fftw_free(orig_freq_buffer);
  fftw_free(test_freq_buffer);
  free(orig_time_buffer);
  free(test_time_buffer);
  free(orig_wmark);
  free_rand();
/*}}}*/
}

