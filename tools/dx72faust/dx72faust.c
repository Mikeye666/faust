/*
  dx72faust.c

  VERSION: 0.0, May 2017
  AUTHOR: Romain Michon - CCRMA, Stanford University / GRAME, Lyon

  DESCRIPTION: A simple program to convert DX7 preset files into Faust
  libraries. It was greatly inspired by dx72csound.c from
  <http://www.parnasse.com/dx72csnd.shtml>

  COMPILATION: typically: `gcc -o dx72faust dx72faust.c`
  A makefile is also available: `make` to build and `make install` to install
*/

#include "stdio.h"
#include <string.h>
#include <stdlib.h>

#define CURRENT_POSITION 1

// Table from d72csound
float FreqTableCoarse[2][32] =
  {
    {0.5,1,2,3,4,5,6,7,8,9,10,11,12,13,
      14,15,16,17,18,19,20,21,22,23,24,
      25,26,27,28,29,30,31},
    {1,10,100,1000,1,10,100,1000,1,10,
      100,1000,1,10,100,1000,1,10,100,
      1000,1,10,100,1000,1,10,100,1000,
      1,10,100,1000}
  };

// Table from d72csound
float FreqTableFine[2][100] =
  {
    {1.00,1.01,1.02,1.03,1.04,1.05,1.06,
      1.07,1.08,1.09,1.10,1.11,1.12,1.13,
      1.14,1.15,1.16,1.17,1.18,1.19,1.20,
      1.21,1.22,1.23,1.24,1.25,1.26,1.27,
      1.28,1.29,1.30,1.31,1.32,1.33,1.34,
      1.35,1.36,1.37,1.38,1.39,1.40,1.41,
      1.42,1.43,1.44,1.45,1.46,1.47,1.48,
      1.49,1.50,1.51,1.52,1.53,1.54,1.55,
      1.56,1.57,1.58,1.59,1.60,1.61,1.62,
      1.63,1.64,1.65,1.66,1.67,1.68,1.69,
      1.70,1.71,1.72,1.73,1.74,1.75,1.76,
      1.77,1.78,1.79,1.80,1.81,1.82,1.83,
      1.84,1.85,1.86,1.87,1.88,1.89,1.90,
      1.91,1.92,1.93,1.94,1.95,1.96,1.97,
      1.98,1.99},
    {1.000,1.023,1.047,1.072,1.096,1.122,
      1.148,1.175,1.202,1.230,1.259,1.288,
      1.318,1.349,1.380,1.413,1.445,1.479,
      1.514,1.549,1.585,1.622,1.660,1.698,
      1.738,1.778,1.820,1.862,1.905,1.950,
      1.995,2.042,2.089,2.138,2.188,2.239,
      2.291,2.344,2.399,2.455,2.512,2.570,
      2.630,2.692,2.716,2.818,2.884,2.951,
      3.020,3.090,3.162,3.236,3.311,3.388,
      3.467,3.548,3.631,3.715,3.802,3.890,
      3.981,4.074,4.169,4.266,4.365,4.467,
      4.571,4.677,4.786,4.898,5.012,5.129,
      5.248,5.370,5.495,5.623,5.754,5.888,
      6.026,6.166,6.310,6.457,6.607,6.761,
      6.918,7.079,7.244,7.413,7.586,7.762,
      7.943,8.128,8.318,8.511,8.718,8.913,
      9.120,9.333,9.550,9.772}
  };

// Structure to store operator-specific parameters
struct dx7_operator {
  unsigned char OP_EG_R1;
  unsigned char OP_EG_R2;
  unsigned char OP_EG_R3;
  unsigned char OP_EG_R4;
  unsigned char OP_EG_L1;
  unsigned char OP_EG_L2;
  unsigned char OP_EG_L3;
  unsigned char OP_EG_L4;
  unsigned char LEV_SCL_BRK_PT;
  unsigned char SCL_LEFT_DEPTH;
  unsigned char SCL_RGHT_DEPTH;
  unsigned char SCL_LEFT_CURVE:2;
  unsigned char SCL_RIGHT_CURVE:6;
  unsigned char OSC_RATE_SCALE:3;
  unsigned char OSC_DETUNE:5;
  unsigned char AMP_MOD_SENS:2;
  unsigned char KEY_VEL_SENS:6;
  unsigned char OUTPUT_LEV;
  unsigned char OSC_MODE:1;
  unsigned char FREQ_COARSE:7;
  unsigned char FREQ_FINE;
} operator[6];

// Structure to store global parameters
struct dx7_globals {
  unsigned char PITCH_EG_R1;
  unsigned char PITCH_EG_R2;
  unsigned char PITCH_EG_R3;
  unsigned char PITCH_EG_R4;
  unsigned char PITCH_EG_L1;
  unsigned char PITCH_EG_L2;
  unsigned char PITCH_EG_L3;
  unsigned char PITCH_EG_L4;
  unsigned char ALGORITHM;
  unsigned char FEEDBACK:3;
  unsigned char OSC_KEY_SYNC:5;
  unsigned char LFO_SPEED;
  unsigned char LFO_DELAY;
  unsigned char LF_PT_MOD_DEP;
  unsigned char LF_AM_MOD_DEP;
  unsigned char SYNC:1;
  unsigned char WAVE:3;
  unsigned char LF_PT_MOD_SNS:4;
  unsigned char TRANSPOSE;
  char NAME[10];
} global;

// function to get rid of spaces in a char
void deblank(char* input)
{
  int i,j;
  char *output=input;
  for (i = 0, j = 0; i<strlen(input); i++,j++){
    if (input[i]!=' '){
      output[j]=input[i];
    }
    else{
      j--;
    }
  }
  output[j]=0;
}

int main(argc,argv)
  int argc;
  char *argv[];
  {
    if(strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0){
      printf("dx72faust: simple program to convert a dx7 .syx preset file into a Faust library.\n");
      printf("Usage: dx72faust presetFile.syx\n");
      exit(0);
    }
    FILE *fpin,*fpout;
    unsigned short x,y,z,SCL_LEFT_CURVE,SCL_RIGHT_CURVE,OSC_DETUNE,OSC_RATE_SCALE;
    unsigned short KEY_VEL_SENS,AMP_MOD_SENS,FREQ_COARSE,OSC_MODE;
    char header[6];
    char out[7];
    int total, count=0,op=0;
    float frequency; // comuted frequency
    char name[11]; // preset name
    int nVoices = 32; // preset file always contain 32 voices

    // loading preset file
    if ( (fpin = fopen(argv[1],"rb")) == 0 ) {
	     printf("Cannot open file %s\n",argv[1]);
	     exit(1);
    }
    fread(&header, 6L, 1, fpin);

    // creating output file (Faust .lib)
    sprintf(out,"dx7_%s.lib",argv[1]);
    if ((fpout=fopen(out,"w")) == 0) {
      printf("Couldn't open %s\n",out);
      exit(1);
    }

    // Writing library header
    fprintf(fpout,"// Faust DX7 presets library created from %s\n\n",argv[1]);
    fprintf(fpout,"import(\"stdfaust.lib\");\n\n");

    // Parsing presets and converting them into Faust functions
    for (count=0;count<nVoices;count++) {
      op=0;
      fread((&(operator[op])), 17L, 6, fpin);
      fread(&global, 16L, 1, fpin);
      fread(&name, 10, 1, fpin);
      name[10] = '\0';
      deblank(name);
      total = 0;

      fprintf(fpout,"// NAME: %s\n",name);
      fprintf(fpout,"// ALGORITHM: %02d\n",global.ALGORITHM+1);

      fprintf(fpout,"dx7_%s(freq,gain,gate) = \n",name);
      fprintf(fpout,"dx.dx7_algo(%i,egR1,egR2,egR3,egR4,egL1,egL2,egL3,egL4,outLevel,keyVelSens,ampModSens,opMode,opFreq,opDetune,opRateScale,feedback,lfoDelay,lfoDepth,lfoSpeed,freq,gain,gate)\nwith{\n",global.ALGORITHM);

      fprintf(fpout,"\tegR1(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_R1);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegR2(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_R2);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegR3(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_R3);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegR4(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_R4);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegL1(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_L1);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegL2(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_L2);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegL3(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_L3);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tegL4(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OP_EG_L4);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\toutLevel(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OUTPUT_LEV);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tkeyVelSens(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].KEY_VEL_SENS);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tampModSens(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].AMP_MOD_SENS);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\topMode(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OSC_MODE);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\topFreq(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        frequency = FreqTableCoarse[operator[op].OSC_MODE][operator[op].FREQ_COARSE] * FreqTableFine[operator[op].OSC_MODE][operator[op].FREQ_FINE];
        fprintf(fpout,"%f",frequency);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\topDetune(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",((int)operator[op].OSC_DETUNE)-7);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\topRateScale(n) = ba.take(n+1,(");
      for (op=5;op>=0;op--) {
        fprintf(fpout,"%d",operator[op].OSC_RATE_SCALE);
        if(op!=0) fprintf(fpout,",");
      }
      fprintf(fpout,"));\n");

      fprintf(fpout,"\tfeedback = %d : dx.dx7_fdbkscalef/(2*ma.PI);\n",global.FEEDBACK);
      fprintf(fpout,"\tlfoDelay = %d;\n",global.LFO_DELAY);
      fprintf(fpout,"\tlfoDepth = %d;\n",global.LF_PT_MOD_DEP);
      fprintf(fpout,"\tlfoSpeed = %d;\n",global.LFO_SPEED);
      fprintf(fpout,"};\n\n");
    }
    fclose(fpout);
    fclose(fpin);
    return 0;
}