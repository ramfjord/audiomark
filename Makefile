CC = gcc
#CFLAGS = -std=gnu99 `pkg-config --libs sndfile` `pkg-config  --libs fftw3`
GSLFLAGS = -lgsl
CFLAGS = -std=gnu99 `pkg-config --libs sndfile` -lfftw3 -lm -lreadline -lgsl -lgslcblas -g

PROGS = gen_audio wembed wtest ssembed sstest GenerateAudio embed test

all: $(PROGS)

gen_audio: gen_audio.c
	$(CC) $? $(CFLAGS) -o $@

wembed: wembed.c watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

embed: embed.c watermark.o wrandom.o
	if [ -e embed ] ; then rm embed; fi
	$(CC) $? $(CFLAGS) -o $@

wtest: wtest.c watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

ssembed: ssembed.c watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

sstest: sstest.c watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o $@

GenerateAudio: GenerateAudio.c
	$(CC) GenerateAudio.c $(CFLAGS) -o GenerateAudio 

test: test.c watermark.o wrandom.o
	$(CC) $? $(CFLAGS) -o test 

clean:
	rm $(PROGS) *.o
