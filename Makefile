CC = gcc
#CFLAGS = -std=gnu99 `pkg-config --libs sndfile` `pkg-config  --libs fftw3`
GSLFLAGS = -lgsl
CFLAGS = -std=gnu99 `pkg-config --libs sndfile` -lfftw3 -lm -lreadline -lgsl -lgslcblas -g

PROGS = embed detect

all: clean $(PROGS)

gen_audio: gen_audio.c
	$(CC) $? $(CFLAGS) -o $@

embed: embed.c w_array_ops.o watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

detect: detect.c w_array_ops.o watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

mytest: mytest.c w_array_ops.o watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

test_psycho: test_psycho.c w_array_ops.o watermark.o wrandom.o my_psycho.o
	$(CC) $? $(CFLAGS) -o $@

#psycho: psychoacoustic.c psychoacoustic.h
#	$(CC) psychoacoustic.c $(CFLAGS) -o $@

#psycho: my_psycho.c my_psycho.h w_array_ops.o watermark.o wrandom.o
#	$(CC) $? $(CFLAGS) -o $@

clean:
	rm -f $(PROGS) *.o
