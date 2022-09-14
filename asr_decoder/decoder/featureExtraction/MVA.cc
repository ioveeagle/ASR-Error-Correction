/*
 * MVA.cc
 *   Basic Mean subtraction, Variance normalization, and ARMA filtering
 *   of an HTK feature file, where the ARMA order is given by the command
 *   line argument. This is modified from a program that does mean subtraction,
 *   variance normalization and ARMA filtering on-line, so here the ARMA
 *   filtering is done sequentially. It can be modified such that the entire
 *   feature file is read into a buffer and then the ARMA filtering is done
 *   entirely in place. Also note that this program is written for little-endian
 *   machines and assumes that the size of short is 2, int is 4 and float is 4.
 *   Such can be modified in obvious ways for machines of different architectures.
 *   We have made sure this is a working version, but it is noted that certain
 *   modifications can be used to speed things up.
 *
 * Written by Chia-ping Chen <chiaping@ssli.ee.washington.edu>
 *
 * Copyright (c) 2003, University of Washington
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any non-commercial purpose
 * and without fee is hereby granted, provided that the above copyright
 * notice appears in all copies.  The University of Washington,
 * Seattle make no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied
 * warranty.
 */

#include<iostream.h>
#include<fstream.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>

/*
 * MV
 *   a function that performs mean subtraction with mean[] and variance
 *   normalization with var[] on data-array data[].
 */

    void MV(float *data, float *mean, float *var, int d) {
      for(int i=0;i<d;i++) {
         data[i] = data[i] - mean[i];
         data[i] /= sqrt(var[i]);
      }
   }

/*
 * A
 *   a function that performs (sequential) ARMA filtering of order m
 *   on the r-th row of buffer mData[], where each row has d elements.
 */

    void A(float *mData, int m, int r, int d) {
      float *sum = new float[d];
      for(int i=0;i<d;i++)
         sum[i] = 0;
      for(int j=0;j<(2*m+1);j++)
         for(int k=0;k<d;k++)
            sum[k] += mData[j*d+k]; 
      for(int i=0;i<d;i++)
         mData[r*d+i] = sum[i]/(2*m+1);
      delete sum;
   }

/*
 * updateBuf
 *   a function that updates the r-th row of buffer mData[],
 *   where each row has d elements, with data-array data[].
 */

    void updateBuf(float *mData, float *data, int r, int d) {
      for(int i=0;i<d;i++)
         mData[r*d+i] = data[i] ; 
   }

/*
 * extractBuf
 *   a function that extracts the r-th row of buffer mData[],
 *   where each row has d elements, to data-array data[].
 */

    void extractBuf(float *mData, float *data, int r, int d) {
      for(int i=0;i<d;i++)
         data[i] = mData[r*d+i];
   }

/*
 * byte-swapping routines
 *   This is for big-endian data to be processed with little-endian architecture.
 *   In addition, the sizes of short, int and float are auumed to be 2, 4 and 4 bytes,
 *   respectively. They should be modified if necessary. 
 */

    void byteSwap(short *s) {
      char v;
      char *tmp = (char *)s ;
      v = *tmp;
      *tmp = *(tmp+1);
      *(tmp+1) = v;
   }
    void byteSwap(int *s) {
      char v;
      char *tmp = (char *)s ;
      v = *(tmp+1);
      *(tmp+1) = *(tmp+2); *(tmp+2) = v;
      v = *(tmp);
      *(tmp) = *(tmp+3);
      *(tmp+3) = v;
   }
    void byteSwapArray(float *s, int d) {
      for(int i=0;i<d;i++) {
         char v;
         char *tmp = (char *)(s+i) ;
         v = *(tmp+1);
         *(tmp+1) = *(tmp+2); *(tmp+2) = v;
         v = *(tmp);
         *(tmp) = *(tmp+3);
         *(tmp+3) = v;
      }
   }

    int main(int argc, char *argv[]) {
      int nSamples, sampPeriod;
      short sampSize, parmKind;
      int t;
      int M = atoi(argv[1]);
      FILE *ifPtr, *ofPtr;
      ifPtr=fopen(argv[2], "r");
      ofPtr=fopen(argv[3], "w");
   printf("%s\n",argv[2]);
   // copy HTK headers
      fread(&nSamples,   sizeof(int),  1, ifPtr);
	  
      fread(&sampPeriod, sizeof(int),  1, ifPtr);
      fread(&sampSize,   sizeof(short), 1, ifPtr);
      fread(&parmKind,   sizeof(short), 1, ifPtr);
	  
      fwrite(&nSamples,   sizeof(int),  1, ofPtr);
      fwrite(&sampPeriod, sizeof(int),  1, ofPtr);
      fwrite(&sampSize,   sizeof(short), 1, ofPtr);
      fwrite(&parmKind,   sizeof(short), 1, ofPtr);
   
   // need to swap for little-endian architecture
      byteSwap(&sampSize); 
	  byteSwap(&parmKind);
      byteSwap(&nSamples); 
	  byteSwap(&sampPeriod);
   printf("sample=%d\n",nSamples);
   printf("%d\n",parmKind);
   //getchar();
      int feaD = sampSize/sizeof(float); // the dimension of the feature vector
      float *data = new float[feaD];
      float *mean = new float[feaD];
      float *var = new float[feaD];
      for(int i=0;i<feaD;i++) {
         mean[i]=0;
         var[i]=0;
      }
   
   /* calculate utterance mean and variance
   *   In per-utterance MVA, the mean and variance are computed from the entire
   * data. So it is needed to go through the data twice. Alternatively, the mean
   * and variance can be updated on-line by partial sample average or in a Bayesian
   * manner. In such cases, the mean and variance updating routines merge
   * into the loop for MVA (below), and the program only go over the data once.
   * The MVA routine is actually modified from on-line implementation.
   *
   */
      for(t=0;t<nSamples;t++) {
         fread(data, sizeof(float), feaD, ifPtr);
         byteSwapArray(data, feaD);
         for(int i=0;i<feaD;i++) {
		 //printf("[%d][%d]=%f\n",t,i,data[i]);
            mean[i] += data[i];
            var[i] += (data[i]*data[i]);
         }
      }
      for(int i=0;i<feaD;i++) {
         mean[i] /= nSamples;
         var[i] = var[i]/nSamples - mean[i]*mean[i];
      }
   
   // rewind
      fseek(ifPtr, 12L, SEEK_SET);
   
   /* post-processing
   *   The following loop is a sequential MVA. Originally the mean and variance
   * can be updated online and a sequential ARMA is used to preserve the low
   * latency. A buffer for a (2m+1)-by-d array of floats is created. In addition, 
   * two indexes inRow and outRow indicate the row in the array to store a new
   * data vector from input file and to be written to output file, respectively.
   * These indexes advance in a round-robin fashion whenever a vector is read-in
   * and when a vector is written out, respectively.
   *   When a vector is read, MV is applied immediately and it is stored in the
   * inRow of data buffer. The ARMA filtering is applied to the vector stored in
   * outRow. The outRow is less than inRow by m (mod 2m+1) because the ARMA
   * filtering needs 2 vectors in the future. 
   *   The boundary condition is that the first and last m vectors are not ARMA-   
   * filtered. This is taken care of by skipping function A.
   *   Graphically, the data buffer is a window that slides through the entire
   * data:
   *      t=0  t=1  t=2  t=3  t=4  t=5 . . . . . . t=T-1
   * 
   * d1    ^    ^    ^    ^    ^    ^               ^
   *       |    |    |    |    |    |               |
   * d2    |    |    |    |    |    |               |
   *       |    |    |    |    |    |               |
   *  .    |    |    |    |    |    |               |
   *  .
   *  .   X[0] X[1] X[2] X[3] X[4] X[5] . . . . . X[T-1]
   *  .
   *  .    |    |    |    |    |    |               |
   *  .    |    |    |    |    |    |               |
   *       |    |    |    |    |    |               |
   *       |    |    |    |    |    |               |
   * dD    v    v    v    v    v    v               v
   * 
   * buf <---------------------->
   * buf       <---------------------->
   * buf            <---------------------->
   * buf                 <---------------------->
   * .                              .
   * .                                .
   * buf                        <---------------------->
   *
   */
   
      float *mData = new float[feaD*(2*M+1)];
      int inRow=0;
      int outRow=0;
      for(t=0;t<nSamples;t++) {
         fread(data, sizeof(float), feaD, ifPtr);
         byteSwapArray(data, feaD);
         MV(data, mean, var, feaD);
         updateBuf(mData, data, inRow, feaD);
         inRow = (inRow+1)%(2*M+1);
         if (t<M) { // not ready to output yet
            continue;
         }
         if (t<2*M) { // output outRow without ARMA (initial frames)
            extractBuf(mData, data, outRow, feaD);
            outRow = (outRow+1)%(2*M+1);
            byteSwapArray(data, feaD);
            fwrite(data, sizeof(float), feaD, ofPtr);
            continue;
         }
         if (t<nSamples) { // output outRow with ARMA (normal case)
            A(mData, M, outRow, feaD);
            extractBuf(mData, data, outRow, feaD);
            outRow = (outRow+1)%(2*M+1);
            byteSwapArray(data, feaD);
            fwrite(data, sizeof(float), feaD, ofPtr);      
            continue;
         }
      }
      for(int i=0;i<M;i++) { // output outRow without ARMA (final frames) 
         extractBuf(mData, data, outRow, feaD);
         outRow = (outRow+1)%(2*M+1);
         byteSwapArray(data, feaD);
         fwrite(data, sizeof(float), feaD, ofPtr);      
      }
   
      fclose(ifPtr);
      fclose(ofPtr);
      delete mean;
      delete var;
      delete data;
      delete mData;
      return(1);
   }
