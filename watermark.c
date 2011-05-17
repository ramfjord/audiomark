#ifndef W_INCLUDES
#define W_INCLUDES

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
#include "w_array_ops.h"

#endif

int		watermark_debug = 1;
watermark	*wmark;


// generates wmark with the default parameters.  Note that message has not been defined - it must be defined in the config
void gen_default_wmark()
{ //{{{
  wmark = (watermark *)malloc(sizeof(watermark));

	wmark->len = WMARK_LEN; // 500, TODO make this changeable?
  wmark->message = malloc(wmark->len); 
  wmark->alpha = .1;
  wmark->schema = MULT_SCHEMA;
	wmark->type = SS_EMBED;
	wmark->processing_gain = 10;
  wmark->key_seed = 15;
	wmark->bpf = 20;
} //}}}

//
// Parse config file to fill out wmark
//
int parse_config(char *config_path)
{ //{{{
	FILE *config;
	if(config = fopen(config_path, "r")){
		char read_buffer[BUFFER_LEN];
		while(1){
			if(fgets(read_buffer, BUFFER_LEN, config) == NULL)
				break;

			if(strncmp(read_buffer, "watermark ", strlen("watermark ")) == 0){
				char *message = read_buffer + strlen("watermark ");
				char delim = *message;  // a delim character should be at the start and end
														// of the watermark: eg watermark "hello" => hello
				message++;
				char *delim2 = NULL;
				if(! (delim2 = index(message,delim))){
					fprintf(stderr,"the watermarked string must be delineated by quotes, or some other character.  Ex. \"hello\", or [hello[, are equivalent, but not [hello]\n");
					exit(1);
				}
				*delim2 = '\0';
				string_repeat(message, wmark->message, wmark->len);
			}

			else if(strncmp(read_buffer, "alpha ", strlen("alpha ")) == 0){
				if(EOF == sscanf(read_buffer, "alpha %lf", &(wmark->alpha)))
					return ERROR;
			}

			else if(strncmp(read_buffer, "schema ", strlen("schema ")) == 0){
				if(ERROR == (wmark->schema = get_schema((read_buffer + strlen("schema ")))))
					return ERROR;
			}

			else if(strncmp(read_buffer, "bpf", strlen("bpf")) == 0){
				if(EOF == sscanf(read_buffer, "bpf%d", &(wmark->bpf)))
					return ERROR;
			}

			else if(strncmp(read_buffer, "seed ", strlen("seed ")) == 0){
				if(EOF == sscanf(read_buffer, "seed %d", &(wmark->key_seed)))
					return ERROR;
			}
			else if(strncmp(read_buffer, "processing_gain ", strlen("processing_gain ")) == 0){
				if(EOF == sscanf(read_buffer, "processing_gain %d", &(wmark->processing_gain)))
					return ERROR;
			}

			else if(strncmp(read_buffer, "type ", strlen("type ")) == 0){
				char *tmp = read_buffer + strlen("type ");
				if(strncmp(tmp, "fh", 2) == 0)
					wmark->type = FH_EMBED;
				else
					wmark->type = SS_EMBED;
			}
		}
	}
} // }}}

// repeats ssmall in slarge until slarge has len characters
// ssmall must be null terminated
void string_repeat(char *ssmall, char *slarge, int len)
{ //{{{
	char *orig = ssmall, *sm = ssmall;
	for(int i = 0; i < len; i++){
		if(*sm == '\0') sm = orig;
		slarge[i] = *sm++;
	}
	slarge[len-1] = '\0';
} // }}}

int get_deinterleave_i(int i, int size, int channels)
{ //{{{
	int c = i % channels;
	int new_i = i / channels;
	return size*c + new_i;
} // }}}
int get_interleave_i(int i, int size, int channels)
{ //{{{
	int c = i / size;
	int new_i = i % size;
	return new_i * channels + c;
} // }}}

// if fwd: [a1 b1 a2 b2 a3 b3] => [a1 a2 a3 b1 b2 b3]
// otherwise, vice versa
// size = num numbers above, channels = num letters above
void interleave_d_array(double *buff, int size, int channels, int fwd)
{ //{{{
	int (*interleave_i)(int,int,int) = (fwd) ? &get_interleave_i : &get_deinterleave_i;
	int n = size*channels;
	char inplace[n];
	memset(inplace,0,n);
	
	for(int i = 1; i < n-1; i++){ // 0 and n-1 start sorted
		if(inplace[i]) continue;

		double swp = buff[i];
		int j = interleave_i(i,size,channels);
		inplace[i] = 1;
		while(j != i){
			swap_d(&swp, &buff[j]);
			
			inplace[j] = 1;
			j = interleave_i(j,size,channels);
			if(j == 0) exit(1);
		}
		swap_d(&swp, &buff[i]);
	}
} // }}}

void deinterleave_channels(double *time_buff, int channels){
	interleave_d_array(time_buff, BUFFER_LEN, channels, 0);
}
void interleave_channels(double *time_buff, int channels){
	interleave_d_array(time_buff, BUFFER_LEN, channels, 1);
}

void free_wmark()
{ // {{{
	free(wmark->message);
	free(wmark);
} // }}}

int get_schema(char *s)
{ //{{{
  if(strstr(s, "plus"))
    wmark->schema = PLUS_SCHEMA;
  else if(strstr(s, "mult"))
    wmark->schema = MULT_SCHEMA;
  else if(strstr(s, "powr"))
    wmark->schema = POWR_SCHEMA;
  else{
    fprintf(stderr, "Invalid Schema: the valid schemas are plus, mult, and powr. \n");
    fprintf(stderr, "plus: v'_i = v_i+aw_i\n");
    fprintf(stderr, "mult: v'_i = v_i(1+aw_i)\n");
    fprintf(stderr, "powr: v'_i = v_i(e*a^w_i)\n");
    return ERROR;
  }
  return wmark->schema;
} // }}}
      
int set_schema(int s)
{ //{{{
  switch (s) {
    case PLUS_SCHEMA: wmark->schema = PLUS_SCHEMA; return PLUS_SCHEMA;
    case MULT_SCHEMA: wmark->schema = MULT_SCHEMA; return MULT_SCHEMA;
    case POWR_SCHEMA: wmark->schema = POWR_SCHEMA; return POWR_SCHEMA;
    default:
      printf("The valid schemas are plus, mult, and powr. \n");
      printf("plus: v'_i = v_i+aw_i\n");
      printf("mult: v'_i = v_i(1+aw_i)\n");
      printf("powr: v'_i = v_i(e*a^w_i)\n");
  }
  return -1;
} // }}}

int set_alpha(int a){
  wmark->alpha = a;
  return a;
}

void print_watermark_info()
{ //{{{
  printf("watermark info:\n");
  printf("\twatermark: >%s<\n",wmark->message);
  printf("\tlen (real %d), %d\n", strlen(wmark->message), wmark->len);
  printf("\talpha: %lf\n",wmark->alpha);
  printf("\tschema: %d\n",wmark->schema);
  printf("\tkey_seed: %u\n",wmark->key_seed);
  printf("\tprocessing_gain: %d\n", wmark->processing_gain);
  printf("\ttype = %s\n", (wmark->type == FH_EMBED) ? "fh" : "ss");
  printf("\tbpf = %d\n", wmark->bpf);
} // }}}

void print_sfile_info(SF_INFO sfinfo)
{ //{{{
  printf("sound file info:\n");
  printf("\tsamplerate: %d\n", sfinfo.samplerate);
  printf("\tchannels: %d\n", sfinfo.channels);
  printf("\tsections: %d\n", sfinfo.sections);
  printf("\tformat: %x\n", sfinfo.format);
  printf("\tframes: %ld\n", (long)sfinfo.frames);
  printf("\tseekable: %d\n", sfinfo.seekable);
} // }}}

double c_to_d(char a)
{ //{{{
  return (double) (((a%2)*2) - 1);
} // }}}

double pow_spect_dens(complex a)
{ //{{{
  return sqrt(pow(creal(a),2)+pow(cimag(a),2));  
} // }}}

//
// Generates an array of noise in buffer
// This is a random sequence of doubles from the normal distribution N(0,1)
// The range is necessary for statistical inference of whether the watermark
// remains
//
void generate_noise(double *buffer, int buffer_len)
{ //{{{
  for(int i = 0; i < buffer_len; i++)
		buffer[i] = norm_double();
} // }}}

// embeds the watermark message into a noise sequence, by keeping track of
// where we are in the message (what has already been put in), flipping the
// sign of each element of noise_seq if the corresponding bit in the watermark
// message
//
void embed_to_noise(double *noise_seq, int noise_len)
{ //{{{
	static int index = 0;
	static int bit_mask = 1;
	char *message = wmark->message;

	for(int i = 0; i < noise_len; i++){
		if(message[index] & bit_mask)
			noise_seq[i] *= -1;

		bit_mask <<= 1;
		if(! (bit_mask & 0xff)){
			bit_mask = 1;
			index++;
			if(index >= wmark->len)
				index = 0;
		}
	}
} // }}}

//
// From freq buff, obtain a subset of indices to which we will add the
// watermark.  The values at these indices corresponds to the vector V from Cox
// et al. 1997.  This is also where the psychoacoustic model would go if it
// were finished.
//
// NOTE: MALLOC's INDICES
int extract_sequence_indices(complex *freq_buff, int len, int **indices)
{ //{{{
	int n = wmark->bpf;
	*indices = get_n_biggest(freq_buff, len, n);
	return n;
} // }}}
