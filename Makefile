CC = gcc
#CFLAGS = -std=gnu99 `pkg-config --libs sndfile` `pkg-config  --libs fftw3`
GSLFLAGS = -lgsl
CFLAGS = -std=gnu99 `pkg-config --libs sndfile` -lfftw3 -lm -lreadline -lgsl -lgslcblas -g

PROGS = embed test

all: $(PROGS)

gen_audio: gen_audio.c
	$(CC) $? $(CFLAGS) -o $@

embed: embed.c w_array_ops.o watermark.o wrandom.o
	if [ -e embed ] ; then rm embed; fi
	$(CC) $? $(CFLAGS) -o $@

test: test.c w_array_ops.o watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o test 

clean:
	rm -f $(PROGS) *.o
