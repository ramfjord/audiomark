#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
  SF_INFO sfinfo;
  SNDFILE *sfile;

  double shit[100];
  for(int i = 0; i < 100; i++) shit[i] = 100;

  sfinfo.format = 0x010005;
  sfinfo.channels = 1;
  sfinfo.samplerate = 11025;
  sfinfo.frames = 0;
  sfinfo.sections = 1;
  sfinfo.seekable = 1;

  printf("samplerate: %d\n", sfinfo.samplerate);
  printf("channels: %d\n", sfinfo.channels);
  printf("sections: %d\n", sfinfo.sections);
  printf("format: %d\n", sfinfo.format);
  printf("frames: %d\n", (int)sfinfo.frames);
  printf("seekable: %d\n", sfinfo.seekable);

  if(!(sfile = sf_open("test.wav", SFM_WRITE, &sfinfo))){
    fprintf(stderr,"error opening the following file for writing: %s\n", "test.wav");
    return 1;
  }


  for(int i = 0; i < 1000; i++)
    sf_write_double(sfile, shit, 100);
}
