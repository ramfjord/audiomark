#define SAMPLE_RATE 44100.0
#define MIN_AMPLITUDE 0.1

void set_sample_rate(double s);
double hz_to_bark(double hz);
int get_critband(double hz);
double get_freq(int i);
int get_i(double freq);
double get_dB(const complex a);
void get_amp_dB(const complex *freq_buf, double *amp_dB);
int is_tonal(const double *amp_dB);
int get_tonal_freqs(const double *amp_dB, int **indices);
double get_inter_mask(double *amp_dB, int *tonal_indices, int num_tonal, int u);
double get_intra_mask(complex *freq_buf, int f, int cb);
double get_quiet_thresh(double f);
double get_noise_mask(double *amp_dB, int *tonal_indices, int num_tonal, int u);
double get_pitch_weight(double spl_excess, double freq);
int get_pitch_saliencies(complex *buffer, int **indices, double **weights);
int main();
