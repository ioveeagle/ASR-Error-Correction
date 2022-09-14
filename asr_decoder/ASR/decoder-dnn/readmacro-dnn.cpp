#include "recog-dnn.h"
//#include <omp.h>
#define DEBUG 0 

#if defined(ASR_SSE2)
  #include <xmmintrin.h>
  #define ALIGN_BYTE 16 // SSE2: 16 bytes (128-bit)
  #define SHIFT_BIT 2
  #define _simd_vec_size 4
  #define _simd_type __m128
  #define _simd_set1_ps _mm_set1_ps
  #define _simd_mul_ps _mm_mul_ps
  #define _simd_add_ps _mm_add_ps
  #define _simd_sqrt_ps _mm_sqrt_ps
  #define _simd_max_ps _mm_max_ps
#elif defined(ASR_AVX)
  #include <immintrin.h>
  #define ALIGN_BYTE 32 // SSE2: 32 bytes (256-bit)
  #define SHIFT_BIT 3 // AVX: 8 single floating-point values
  #define _simd_vec_size 8
  #define _simd_type __m256
  #define _simd_set1_ps _mm256_set1_ps
  #define _simd_mul_ps _mm256_mul_ps
  #define _simd_add_ps _mm256_add_ps
  #define _simd_sqrt_ps _mm256_sqrt_ps
  #define _simd_max_ps _mm256_max_ps
#elif defined(ASR_NEON)
  #include <arm_neon.h>
  #define ALIGN_BYTE 16 // SSE2: 16 bytes (128-bit)
  #define SHIFT_BIT 2
  #define _simd_vec_size 4
  #define _simd_type float32x4_t
  #define _simd_set1_ps vdupq_n_f32
  #define _simd_mul_ps vmulq_f32
  #define _simd_add_ps vaddq_f32
  /* NEON has no sqrt(x) function, so we use x/sqrt(x) to calculate it */
  #define _simd_sqrt_ps(x) vmulq_f32(x, vrsqrteq_f32(x)) 
  #define _simd_max_ps vmaxq_f32
#else
  #include <xmmintrin.h>
  #define ALIGN_BYTE 16
#endif



int DNN::readMacroDNN(char *fname){
	//UTILITY util;
	int i, j, m, numRead;
	size_t st;
	char infoStr[DNNSTRLEN], dnnStr[DNNSTRLEN], dnnCompStr[DNNSTRLEN];
	FILE *in=util.openFile(fname, "rb");

	if( in != NULL ){
		// read in HMM information
		st=fread(infoStr, sizeof(char), DNNSTRLEN, in);
		st=fread(&numTriphone, sizeof(int), 1, in);

		MMF=new MASTERMACROFILE[numTriphone];
		p2MMF=new MASTERMACROFILE*[numTriphone];
		tm=new TIEDMATRIX[numTriphone];
		p2tm=new TIEDMATRIX*[numTriphone];

		for( i=0; i<numTriphone; i++){
			st=fread(MMF[i].str, sizeof(char)*DNNSTRLEN, 1, in);
			st=fread(&MMF[i].stateNum, sizeof(int), 1, in);
			MMF[i].tiedStateIndex = new int[MMF[i].stateNum-2];
			st=fread(MMF[i].tiedStateIndex, sizeof(int), MMF[i].stateNum-2, in);
			st=fread(&MMF[i].tiedMatrixIndex, sizeof(int),1,in);
		}

		for( i=0; i<numTriphone; i++)
			p2MMF[i]=MMF+i;
	
		for( i=0; i<numTriphone; i++){
			st=fread(tm[i].str, sizeof(char)*DNNSTRLEN, 1, in);
			st=fread(&tm[i].stateNum, sizeof(int), 1, in);
			tm[i].transProb = new float[tm[i].stateNum*tm[i].stateNum];
			st=fread(tm[i].transProb, sizeof(float), tm[i].stateNum*tm[i].stateNum, in);
		}

		for( i=0; i<numTriphone; i++)
			p2tm[i]=tm+i;
		// read in DNN information
		st=fread(infoStr, sizeof(char), DNNSTRLEN, in);
		st=fread(&numDnnComponent, sizeof(int), 1, in);
		
		st=fread(infoStr, sizeof(char), DNNSTRLEN, in);
		if( strcmp(infoStr, "SPLICE")==0 ){
			st=fread(&streamWidth, sizeof(int), 1, in);
			st=fread(&sizeBlock, sizeof(int), 1, in);
		}else{
			printf("[ERROR] Cannot read SpliceComponent information.");
			exit(1);
		}

		DNNAM = new DNNCOMPONENT[numDnnComponent];
		for( i=0; i<numDnnComponent; i++){
			DNNAM[i].W=NULL;
			DNNAM[i].B=NULL;
			DNNAM[i].G=NULL;
			DNNAM[i].VQ=NULL;
		}

		for( i=0; i<numDnnComponent; i++){
			st=fread(infoStr, sizeof(char), DNNSTRLEN, in);
			st=fread(&DNNAM[i].inDim, sizeof(int), 1, in);
			st=fread(&DNNAM[i].outDim, sizeof(int), 1, in);
		
			if( DEBUG ){
				printf("[INFO] DNN Layer %d: type:%s ", i, infoStr);
				printf("inDim:%d outDim:%d\n", DNNAM[i].inDim, DNNAM[i].outDim);
			}

			if( strcmp(infoStr, "AFFINE_TRANSFORM")==0 ){
				DNNAM[i].type = AFFINE_TRANSFORM;
				
				#if defined(ASR_SSE2)||defined(ASR_AVX)||defined(ASR_NEON)
				/* SSE2 codes, make input dimension as a multiple of 4 */				
				int loopA, loopB;
				loopA=DNNAM[i].inDim>>SHIFT_BIT;
				loopB=DNNAM[i].inDim-(loopA<<SHIFT_BIT);
					
				if( loopB!=0 ){
					pad_feature = (1<<SHIFT_BIT)-loopB;
					numRead = DNNAM[i].inDim;
					DNNAM[i].W = (SCORETYPE*)util.aligned_malloc( sizeof(SCORETYPE)*(DNNAM[i].inDim+pad_feature)*DNNAM[i].outDim, ALIGN_BYTE);
					for(m=0; m<DNNAM[i].outDim; m++){
						st=fread(&DNNAM[i].W[m*(DNNAM[i].inDim+pad_feature)], sizeof(float), numRead, in);
						for(int n=0; n<pad_feature; n++)
							DNNAM[i].W[m*(DNNAM[i].inDim+pad_feature)+DNNAM[i].inDim+n]=0.0f;
					}	
				}else{
				
					pad_feature=0;
					numRead = DNNAM[i].inDim;
					DNNAM[i].W = (SCORETYPE*)util.aligned_malloc( sizeof(SCORETYPE)*(DNNAM[i].inDim*DNNAM[i].outDim), ALIGN_BYTE );
					for(m=0; m<DNNAM[i].outDim; m++)
						st=fread(&DNNAM[i].W[m*DNNAM[i].inDim], sizeof(float), numRead, in);
				//printf("readin %ld data for layer %d\n", st, i);
				//printf("%f %f\n", DNNAM[i].W[0], DNNAM[i].W[numRead-1]);
				}

				DNNAM[i].inDim = DNNAM[i].inDim+pad_feature;
				#else	
				/* CPU codes */
				numRead = DNNAM[i].inDim*DNNAM[i].outDim;
				DNNAM[i].W = (SCORETYPE*)util.aligned_malloc( sizeof(SCORETYPE)*(DNNAM[i].inDim*DNNAM[i].outDim), ALIGN_BYTE );
				st=fread(DNNAM[i].W, sizeof(float), numRead, in);
				#endif

				numRead = DNNAM[i].outDim;
				//DNNAM[i].B = new float [ numRead ];
				DNNAM[i].B = (SCORETYPE*)util.aligned_malloc(sizeof(SCORETYPE)*numRead, ALIGN_BYTE);
                                st=fread(DNNAM[i].B, sizeof(float), numRead, in);
				
				DNNAM[i].G = NULL;

				#if defined(LAZY)
				lastAffineLayer=i;
				#endif
			}else if( strcmp(infoStr, "VQ_AFFINE_TRANSFORM")==0 ){
				DNNAM[i].type = VQ_AFFINE_TRANSFORM;
				/* CPU codes */
				st=fread(&DNNAM[i].inDim, sizeof(int), 1, in); // CodeWordSize, say 256
				st=fread(&DNNAM[i].outDim, sizeof(int), 1, in); // CodeWordDim, say 200
				numRead = DNNAM[i].inDim*DNNAM[i].outDim;
				DNNAM[i].W = (SCORETYPE*)util.aligned_malloc( sizeof(SCORETYPE)*numRead, ALIGN_BYTE );
				st=fread(DNNAM[i].W, sizeof(SCORETYPE), numRead, in);
				DNNAM[i].vqDim=DNNAM[i].outDim;
				DNNAM[i].vqSize=DNNAM[i].inDim;
								
				st=fread(&DNNAM[i].outDim, sizeof(int), 1, in); // CodeWordIndexSize, say 2000
				st=fread(&DNNAM[i].inDim, sizeof(int), 1, in); // CodeWordIndexDim, say 1
				numRead = DNNAM[i].inDim*DNNAM[i].outDim;
				DNNAM[i].G = (int*)util.aligned_malloc( sizeof(int)*numRead, ALIGN_BYTE );
				st=fread(DNNAM[i].G, sizeof(int), numRead, in);

				numRead = DNNAM[i].outDim; // dim(Bias) = CodeWordIndexSize
				DNNAM[i].B = (SCORETYPE*)util.aligned_malloc(sizeof(SCORETYPE)*numRead, ALIGN_BYTE);
                                st=fread(DNNAM[i].B, sizeof(float), numRead, in);

				numRead = DNNAM[i].vqSize*DNNAM[i].inDim; // CodeWordSize*CodeWordIndexDim
				DNNAM[i].VQ = (SCORETYPE*)util.aligned_malloc( sizeof(SCORETYPE)*numRead, ALIGN_BYTE );
				for(int m=0; m<numRead; m++)
					DNNAM[i].VQ[m]=0.0f;

			}else if( strcmp(infoStr, "PNORM")==0 ){
				DNNAM[i].type = PNORM;
			}else if( strcmp(infoStr, "NORMALIZE")==0 ){
				DNNAM[i].type = NORMALIZE;
			}else if( strcmp(infoStr, "RELU")==0 ){
				DNNAM[i].type = RELU;
			}else if( strcmp(infoStr, "FIXEDSCALE")==0 ){
				DNNAM[i].type = FIXEDSCALE;

				numRead = DNNAM[i].inDim;
				//DNNAM[i].W = new float [ numRead ];
				DNNAM[i].W = (SCORETYPE*)util.aligned_malloc(sizeof(SCORETYPE)*numRead, ALIGN_BYTE);
				st=fread(DNNAM[i].W, sizeof(float), numRead, in);
			}else if( strcmp(infoStr, "SOFTMAX")==0 ){
				DNNAM[i].type = SOFTMAX;
			}else if( strcmp(infoStr, "SUMGROUP")==0 ){
				DNNAM[i].type = SUMGROUP;

				numRead = DNNAM[i].outDim;
				DNNAM[i].G = new int [ numRead ];
                                st=fread(DNNAM[i].G, sizeof(int), numRead, in);
			}else{
				printf("[ERROR] Unrecognized DNN component string! %s\n", infoStr);
				exit(1);
			}
			
		} // finish reading DNN components
	
		st=fread(infoStr, sizeof(char), DNNSTRLEN, in);
		if( strcmp(infoStr, "PRIOR")==0 ){
			st=fread(&numPrior, sizeof(int), 1, in);
			Prior = new float [ numPrior ];
			st=fread(Prior, sizeof(float), numPrior, in);
			for(i=0; i<numPrior; i++)
				Prior[i]=1/exp(Prior[i]);

			if( DEBUG )
				printf("[INFO] Read %d Priors\n", numPrior);			
		}else{
			printf("[ERROR] Cannot read priors in DNN!\n");
			exit(1);
		}
	
	}else{
		printf("Cannot open DNN macro!\n");
		exit(1);
	}
	fclose(in);

	#if defined(LAZY)
	in=util.openFile("lazy.idx", "rb");
	if( in != NULL ){
		st=fread(&numLazyIdx, sizeof(int), 1, in);
		lazyStateIdx = (int*)util.aligned_malloc( sizeof(int)*numLazyIdx, ALIGN_BYTE);
		st=fread(lazyStateIdx, sizeof(int), numLazyIdx, in);

		lazyStateMask = (int*)util.aligned_malloc(sizeof(int)*numPrior, ALIGN_BYTE);
		for(i=0; i<numPrior; i++)
			lazyStateMask[i]=-1;
		for(i=0; i<numLazyIdx; i++)
			lazyStateMask[lazyStateIdx[i]]=i;

		// modify state idx
		for(i=0; i<numTriphone; i++)
			for(j=0; j<MMF[i].stateNum-2; j++)
				if( lazyStateMask[MMF[i].tiedStateIndex[j]] >= 0 )
					MMF[i].tiedStateIndex[j] = lazyStateMask[MMF[i].tiedStateIndex[j]];

		// modify W and b of last affine transform
		for(i=0; i<numPrior; i++){
			if( lazyStateMask[i]>=0 ){
				for(j=0; j<DNNAM[lastAffineLayer].inDim; j++)
					DNNAM[lastAffineLayer].W[lazyStateMask[i]*DNNAM[lastAffineLayer].inDim+j]=DNNAM[lastAffineLayer].W[i*DNNAM[lastAffineLayer].inDim+j];
				DNNAM[lastAffineLayer].B[lazyStateMask[i]]=DNNAM[lastAffineLayer].B[i];
			}
		}
		DNNAM[lastAffineLayer].outDim=numLazyIdx;
		// modify Prior vector and count
		for(i=0; i<numPrior; i++)
			if( lazyStateMask[i]>=0 )
				Prior[ lazyStateMask[i] ] = Prior[i];
		numPrior = numLazyIdx;

		for(i=lastAffineLayer+1; i<numDnnComponent; i++){
			if(DNNAM[i].type == SOFTMAX){
				DNNAM[i].inDim=numLazyIdx;
				DNNAM[i].outDim=numLazyIdx;
			}
		}

		
	}else{
		printf("Cannot open 'lazy.idx' for lazy evaluation!\n");
		exit(1);
	}
	fclose(in);
	#endif
	

	initializeDNN();

	return numDnnComponent;
}

int DNN::initializeDNN(void){
	int numLayerOut = numDnnComponent+1;
	int loopA, loopB, n_element;
	
	//tmpOutputDNN = new SCORETYPE*[numLayerOut];
	tmpOutputDNN = (SCORETYPE**)util.aligned_malloc(sizeof(SCORETYPE*)*numLayerOut, ALIGN_BYTE);
	tmpOutputDNN[0]=NULL;
	/*
	tmpOutputDNN[0] = new SCORETYPE[DNNAM[0].inDim];

	for(int j=0; j<DNNAM[0].inDim; j++)
		tmpOutputDNN[0][j]=0;
	*/
	for(int i=1; i<numLayerOut; i++){
		//tmpOutputDNN[i] = new SCORETYPE[DNNAM[i-1].outDim];
		#if defined(ASR_SSE2)||defined(ASR_AVX)||defined(ASR_NEON)
		loopA=DNNAM[i-1].outDim>>SHIFT_BIT;
		loopB=DNNAM[i-1].outDim-(loopA<<SHIFT_BIT);
		(loopB==0)?n_element=DNNAM[i-1].outDim:n_element=(loopA+1)<<SHIFT_BIT;
		#else
		n_element=DNNAM[i-1].outDim;
		#endif
		tmpOutputDNN[i] = (SCORETYPE*)util.aligned_malloc(sizeof(SCORETYPE)*n_element, ALIGN_BYTE);
		for(int j=0; j<n_element; j++)
			tmpOutputDNN[i][j]=0.0f;
	}

	frameScore = new SCORETYPE*[MAX_FRAME_NUM];
	for(int j=0; j<MAX_FRAME_NUM; j++){
		frameScore[j] = new SCORETYPE[DNNAM[numDnnComponent-1].outDim];
		memset(frameScore[j], 0, sizeof(SCORETYPE)*DNNAM[numDnnComponent-1].outDim);
	}

	#if defined(ASR_SSE2)||defined(ASR_AVX)||defined(ASR_NEON)
	int dim;
	int i=DNNAM[0].inDim>>SHIFT_BIT;
	int j=DNNAM[0].inDim-(i<<SHIFT_BIT);
	(j==0)?dim=(i<<SHIFT_BIT):dim=((i+1)<<SHIFT_BIT);
	//buf_fea = new SCORETYPE [dim];
	buf_fea = (SCORETYPE*)util.aligned_malloc(sizeof(SCORETYPE)*dim, ALIGN_BYTE);
	memset(buf_fea, 0, sizeof(SCORETYPE)*dim);
	//printf("dim:%d\n", dim);
	#endif


	return 0;
}

#if defined(ASR_SSE2)||defined(ASR_AVX)||defined(ASR_NEON)
void DNN::propagateSIMD(SCORETYPE *fea, int startFrame, int numFrame, int numFeaDim){
	_simd_type *pVecA=NULL, *pVecB=NULL, *pVecC=NULL, *pVecZ=NULL;
	_simd_type VecD;
	SCORETYPE vec[_simd_vec_size] __attribute__ ((aligned(ALIGN_BYTE)));
	int i, j, k, l, loopA, loopB;

	SCORETYPE tmp=0;

	/*
	if( numFrame*numFeaDim+pad_feature != DNNAM[0].inDim ){
		printf("[ERROR] input dimension to propagate() mismatch input dimension of DNN\n");
		printf("\tnumFrame*numFeaDim=%d\tDNNAM[0].inDim=%d\n", numFrame*numFeaDim, DNNAM[0].inDim);
		exit(1);
	}
	*/

	memcpy(buf_fea, fea+startFrame*numFeaDim, sizeof(SCORETYPE)*DNNAM[0].inDim);
	//printf("feature: %d %d %d %d\n", DNNAM[0].inDim, startFrame*numFeaDim, startFrame, numFeaDim);
	tmpOutputDNN[0]=buf_fea;
	//tmpOutputDNN[0]=fea+startFrame*numFeaDim;

	/* clear all outputs between layers to zero */
	for(l=1; l<numDnnComponent+1; l++){
		loopA=DNNAM[l-1].outDim>>SHIFT_BIT;
                loopB=DNNAM[l-1].outDim-(loopA<<SHIFT_BIT);
		pVecA=(_simd_type*)tmpOutputDNN[l];
		for(j=0; j<loopA; j++){
			*pVecA=_simd_set1_ps(0.0f);
			pVecA++;
		}

		for(j=DNNAM[l-1].outDim-loopB; j<DNNAM[l-1].outDim; j++)
			tmpOutputDNN[l][j]=0;		
		//printf("%d/%d\n", l, numDnnComponent);
	}

	/* propagate through different types of layers */
	for(l=0; l<numDnnComponent; l++){

		pVecC=(_simd_type*)vec;
		*pVecC=_simd_set1_ps(0.0f);

		if( DNNAM[l].type == AFFINE_TRANSFORM ){				
			loopA=DNNAM[l].inDim>>SHIFT_BIT;
			loopB=DNNAM[l].inDim-(loopA<<SHIFT_BIT);

			pVecA=(_simd_type*)DNNAM[l].W;
			pVecB=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)vec;
			*pVecC=_simd_set1_ps(0.0f);		

			/* calculate W*a */
			for( j=0; j<DNNAM[l].outDim; j++){
				for( k=0; k<loopA; k++ ){
					(*pVecC)+=_simd_mul_ps(*pVecA, *pVecB);
					pVecA++;
					pVecB++;
				}
				#if defined(ASR_AVX)
				tmpOutputDNN[l+1][j]=(vec[0]+vec[1])+(vec[2]+vec[3])+(vec[4]+vec[5])+(vec[6]+vec[7]);
				#else
				tmpOutputDNN[l+1][j]=(vec[0]+vec[1])+(vec[2]+vec[3]);
				#endif
				
				for( k=DNNAM[l].inDim-loopB; k<DNNAM[l].inDim; k++)
					tmpOutputDNN[l+1][j]+=tmpOutputDNN[l][k]*DNNAM[l].W[j*DNNAM[l].inDim+k];

				pVecB=(_simd_type*)tmpOutputDNN[l]; // point back to feature begin
				*pVecC=_simd_set1_ps(0.0f);
			}

			if(DEBUG){
				printf("SSE2 layer:%d %d inDim:%d\n", l, DNNAM[l].type, DNNAM[l].inDim);
				for(j=0; j<DNNAM[l].outDim; j++)
					printf("[%d] %f ",j, tmpOutputDNN[l+1][j]);
				printf("\n");
			}

			/* calculate plus B */
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
			loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);
			pVecA=(_simd_type*)DNNAM[l].B;
			pVecB=(_simd_type*)tmpOutputDNN[l+1];

			for(j=0; j<loopA; j++){
				*pVecB=_simd_add_ps(*pVecA, *pVecB);
				pVecA++;
				pVecB++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]+=DNNAM[l].B[j];

		}else if( DNNAM[l].type == VQ_AFFINE_TRANSFORM ){
			
			int vqIdx=0;

			pVecA=(_simd_type*)DNNAM[l].W;
			pVecB=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)vec;
			*pVecC=_simd_set1_ps(0.0f);

			/* calculate W*a */
			#pragma omp parallel for
			loopA=DNNAM[l].vqDim>>SHIFT_BIT;
			loopB=DNNAM[l].vqDim-(loopA<<SHIFT_BIT);
			for( j=0; j<DNNAM[l].vqSize; j++){
				for( k=0; k<loopA; k++ ){
                                        (*pVecC)+=_simd_mul_ps(*pVecA, *pVecB);
                                        pVecA++;
                                        pVecB++;
                                }
				#if defined(ASR_AVX)
				DNNAM[l].VQ[j]=(vec[0]+vec[1])+(vec[2]+vec[3])+(vec[4]+vec[5])+(vec[6]+vec[7]);
				#else
				DNNAM[l].VQ[j]=(vec[0]+vec[1])+(vec[2]+vec[3]);
				#endif

				for( k=DNNAM[l].vqDim-loopB; k<DNNAM[l].vqDim; k++)
					DNNAM[l].VQ[j]+=tmpOutputDNN[l][k]*DNNAM[l].W[j*DNNAM[l].vqDim+k];

				pVecB=(_simd_type*)tmpOutputDNN[l]; // point back to feature begin
                                *pVecC=_simd_set1_ps(0.0f);
			}

			/* distribute calculated values */			
			for( j=0; j<DNNAM[l].outDim; j++){
				tmpOutputDNN[l+1][j]=DNNAM[l].VQ[DNNAM[l].G[j]-1];
			}

			/* zeroing DNNAM[l].VQ */
			loopA=DNNAM[l].vqSize>>SHIFT_BIT;
			loopB=DNNAM[l].vqSize-(loopA<<SHIFT_BIT);
			pVecC=(_simd_type*)DNNAM[l].VQ;
			for( j=0; j<loopA; j++)
				*pVecC=_simd_set1_ps(0.0f);

			for( j=DNNAM[l].vqSize-loopB; j<DNNAM[l].vqSize; j++ )
				DNNAM[l].VQ[j]=0.0f;
				
			/* calculate plus B */
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
			loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);
			pVecA=(_simd_type*)DNNAM[l].B;
			pVecB=(_simd_type*)tmpOutputDNN[l+1];

			for(j=0; j<loopA; j++){
				*pVecB=_simd_add_ps(*pVecA, *pVecB);
				pVecA++;
				pVecB++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]+=DNNAM[l].B[j];

			if(DEBUG){
				printf("SSE2 VQ layer:%d %d inDim:%d\n", l, DNNAM[l].type, DNNAM[l].inDim);
				for(j=0; j<DNNAM[l].inDim; j++)
					printf("[%d] %f ",j, tmpOutputDNN[l][j]);
				printf("\n");
			}
		}else if( DNNAM[l].type == SOFTMAX ){
			tmp=0;
			for(j=0; j<DNNAM[l].outDim; j++){
				tmpOutputDNN[l+1][j]=exp(tmpOutputDNN[l][j]);
				tmp+=tmpOutputDNN[l+1][j];
			}
			tmp=1.0f/tmp;

			loopA=DNNAM[l].outDim>>SHIFT_BIT;
			loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);
			pVecB=(_simd_type*)tmpOutputDNN[l+1];
			*pVecC=_simd_set1_ps(tmp);
			for(j=0; j<loopA; j++){
				*pVecB=_simd_mul_ps(*pVecB, *pVecC);
				pVecB++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]*=tmp;

			/* floor small value to 1e-20 */
			for(j=0; j<DNNAM[l].outDim; j++)
				if( tmpOutputDNN[l+1][j] < 1e-20 )
					tmpOutputDNN[l+1][j] = 1e-20;
			
		}else if( DNNAM[l].type == PNORM ){
			int pnorm_dim = DNNAM[l].inDim/DNNAM[l].outDim;

			/* calculate x^2 */
			loopA=DNNAM[l].inDim>>SHIFT_BIT;
			loopB=DNNAM[l].inDim-(loopA<<SHIFT_BIT);
			pVecC=(_simd_type*)tmpOutputDNN[l];
	
			for(j=0; j<loopA; j++){
				*pVecC=_simd_mul_ps(*pVecC, *pVecC);
				pVecC++;
			}
			for(j=DNNAM[l].inDim-loopB; j<DNNAM[l].inDim; j++){
				tmpOutputDNN[l][j]=tmpOutputDNN[l][j]*tmpOutputDNN[l][j];
			}

			/* sum x^2 in the same group */
			if(DEBUG){
				printf("PNORM 1: inDim:%d pnormDim:%d\n", DNNAM[l].inDim, pnorm_dim);
				for(j=0; j<DNNAM[l].inDim; j++){ printf(" [%d] %f", j, tmpOutputDNN[l][j]); }
				printf("\n");
			}
			for(j=0; j<DNNAM[l].inDim/pnorm_dim; j++){
				for(k=0; k<pnorm_dim; k++)
					tmpOutputDNN[l+1][j]+=tmpOutputDNN[l][j*pnorm_dim+k];
			}
			if(DEBUG){
				printf("PNORM 2: inDim:%d pnormDim:%d\n", DNNAM[l].inDim, pnorm_dim);
				for(j=0; j<DNNAM[l].inDim/pnorm_dim; j++){ printf(" %f", tmpOutputDNN[l+1][j]); }
				printf("\n");
			}
			/* calculate sqrt(x) */
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
                        loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);
			pVecC=(_simd_type*)tmpOutputDNN[l+1];
			for(j=0; j<loopA; j++){
				*pVecC=_simd_sqrt_ps(*pVecC);
				pVecC++;
			}

			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]=sqrt(tmpOutputDNN[l+1][j]);
			
		}else if( DNNAM[l].type == NORMALIZE ){
			/* normalize with root-mean-square */
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
                        loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);
			tmp=(SCORETYPE)1.0f/DNNAM[l].outDim;

			/* calculate a=(x^2)/N */
			pVecA=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)tmpOutputDNN[l+1];
			VecD=_simd_set1_ps(tmp);
			for(j=0; j<loopA; j++){
				*pVecC=_simd_mul_ps(*pVecA, *pVecA);
				*pVecC=_simd_mul_ps(*pVecC, VecD);
				pVecA++;
				pVecC++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]*tmpOutputDNN[l][j]*tmp;

			/* calculate b=sum(a) */
			tmp=0;
			pVecA=(_simd_type*)tmpOutputDNN[l+1];
			pVecC=(_simd_type*)vec;
			*pVecC=_simd_set1_ps(0.0f);
			for(j=0; j<loopA; j++){
				*pVecC=_simd_add_ps(*pVecA, *pVecC);
				pVecA++;
			}
			#if defined(ASR_AVX)
			tmp=(vec[0]+vec[1])+(vec[2]+vec[3])+(vec[4]+vec[5])+(vec[6]+vec[7]);
			#else
			tmp=(vec[0]+vec[1])+(vec[2]+vec[3]);
			#endif
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmp+=tmpOutputDNN[l+1][j];
				
			/* calculate c=x/sqrt(b) */
			tmp=1.0f/sqrt(tmp);
			pVecA=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)tmpOutputDNN[l+1];

			if( tmp<1.0f )
				VecD=_simd_set1_ps(tmp);
			else
				VecD=_simd_set1_ps(1.0f);

			for(j=0; j<loopA; j++){
				*pVecC=_simd_mul_ps(*pVecA, VecD);
				pVecA++;
				pVecC++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]*tmp;

		}else if( DNNAM[l].type == RELU ){
			
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
                        loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);

			pVecA=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)tmpOutputDNN[l+1];
			VecD=_simd_set1_ps(0.0f);
			for(j=0; j<loopA; j++){
				*pVecC=_simd_max_ps(*pVecA, VecD);
				pVecA++;
				pVecC++;
			}

			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				(tmpOutputDNN[l][j]>0.0f)?tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]:tmpOutputDNN[l+1][j]=0.0f;

		}else if( DNNAM[l].type == FIXEDSCALE ){
			
			loopA=DNNAM[l].outDim>>SHIFT_BIT;
                        loopB=DNNAM[l].outDim-(loopA<<SHIFT_BIT);

			pVecA=(_simd_type*)DNNAM[l].W;
			pVecB=(_simd_type*)tmpOutputDNN[l];
			pVecC=(_simd_type*)tmpOutputDNN[l+1];

			for(j=0; j<loopA; j++){
				*pVecC=_simd_mul_ps(*pVecA, *pVecB);
				pVecA++;
				pVecB++;
				pVecC++;
			}
			for(j=DNNAM[l].outDim-loopB; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]+=DNNAM[l].W[j]*tmpOutputDNN[l][j];


		}else if( DNNAM[l].type == SUMGROUP ){
			/* no need to use SIMD here because the number of elements in each group is 1 or 2 */
			int idx=0;
			for(j=0; j<DNNAM[l].outDim; j++){
				for(k=0; k<DNNAM[l].G[j]; k++)
					tmpOutputDNN[l+1][j]+=tmpOutputDNN[l][idx+k];
				idx+=DNNAM[l].G[j];
			}
		}

		if( DEBUG && DNNAM[l].type == AFFINE_TRANSFORM && (startFrame%100==0) ){
			SCORETYPE sum=0;
			printf("Layer %d Output\n", l);
			for(i=0; i<DNNAM[l].outDim; i++){
				printf("%e ", tmpOutputDNN[l+1][i]);
				sum+=tmpOutputDNN[l+1][i];
			}
			printf("SUM:%f\n", sum);
		}
		
	}



	/* save output of last layer (posterior probability) */
	if( numPrior != DNNAM[numDnnComponent-1].outDim){
		printf("[ERROR] numPrior != outDim_last_layer!\n");
		exit(1);
	}

	if( applyPrior ){
		/* calculate p(c'|x)=p(c|x)/p(c) 
		   Prior here is 1/p(c) see readMacroDNN() load prior section
		*/
		loopA=DNNAM[numDnnComponent-1].outDim>>SHIFT_BIT;
		loopB=DNNAM[numDnnComponent-1].outDim-(loopA<<SHIFT_BIT);

		pVecA=(_simd_type*)tmpOutputDNN[numDnnComponent];
		pVecB=(_simd_type*)Prior;

		for(j=0; j<loopA; j++){
			*pVecA=_simd_mul_ps(*pVecA, *pVecB);
			pVecA++;
			pVecB++;
		}
		for(j=DNNAM[numDnnComponent-1].outDim-loopB; j<DNNAM[numDnnComponent-1].outDim; j++)
			tmpOutputDNN[numDnnComponent][j]=tmpOutputDNN[numDnnComponent][j]*Prior[j];

		/* sum of p(c'|x) */
		tmp=0.0f;
		pVecA=(_simd_type*)tmpOutputDNN[numDnnComponent];
		pVecC=(_simd_type*)vec;
		*pVecC=_simd_set1_ps(0.0f);
		for(j=0; j<loopA; j++){
			*pVecC=_simd_add_ps(*pVecA, *pVecC);
			pVecA++;
		}

		#if defined(ASR_AVX)
		tmp=(vec[0]+vec[1])+(vec[2]+vec[3])+(vec[4]+vec[5])+(vec[6]+vec[7]);
		#else
		tmp=(vec[0]+vec[1])+(vec[2]+vec[3]);
		#endif
		for(j=DNNAM[numDnnComponent-1].outDim-loopB; j<DNNAM[numDnnComponent-1].outDim;j++)
			tmp+=tmpOutputDNN[numDnnComponent][j];
		
		/* normalize p(c'|x) */
		tmp=1.0f/tmp;
		pVecA=(_simd_type*)tmpOutputDNN[numDnnComponent];
		pVecC=(_simd_type*)vec;
		*pVecC=_simd_set1_ps(tmp);
		for(j=0; j<loopA; j++){
			*pVecA=_simd_mul_ps(*pVecA, *pVecC);
			pVecA++;
		}
		for(j=DNNAM[numDnnComponent-1].outDim-loopB; j<DNNAM[numDnnComponent-1].outDim;j++)
			tmpOutputDNN[numDnnComponent][j]=tmpOutputDNN[numDnnComponent][j]*tmp;
	}

	for(i=0; i<DNNAM[numDnnComponent-1].outDim; i++){
		if( applyLog ){			
			frameScore[startFrame][i]=log(tmpOutputDNN[numDnnComponent][i]);
		}else{
			frameScore[startFrame][i]=tmpOutputDNN[numDnnComponent][i];
		}
	
	}
}	
#else
void DNN::propagate(SCORETYPE *fea, int startFrame, int numFrame, int numFeaDim){
	int i, j, k, l, z;
	SCORETYPE tmp=0;

	if( numFrame*numFeaDim != DNNAM[0].inDim ){
		printf("[ERROR] input dimension to propagate() mismatch input dimension of DNN\n");
		exit(1);
	}

	tmpOutputDNN[0]=fea+startFrame*numFeaDim;

	/* clear all outputs between layers to zero */
	for(l=1; l<numDnnComponent+1; l++)
		for(j=0; j<DNNAM[l-1].outDim; j++)
			tmpOutputDNN[l][j]=0;

	for(l=0; l<numDnnComponent; l++){
		if( DNNAM[l].type == AFFINE_TRANSFORM ){
			#pragma omp parallel for
			for( j=0; j<DNNAM[l].outDim; j++){
				for( k=0; k<DNNAM[l].inDim; k++ )
					tmpOutputDNN[l+1][j]+=tmpOutputDNN[l][k]*DNNAM[l].W[j*DNNAM[l].inDim+k];
			}

			if(DEBUG){
				printf("CPU\n");
				for(j=0; j<DNNAM[l].inDim; j++)
					printf("%f ", tmpOutputDNN[l][j]);
				printf("\n");
			}

			#pragma omp parallel for
			for(j=0; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]+=DNNAM[l].B[j];

		}else if( DNNAM[l].type == VQ_AFFINE_TRANSFORM ){
			int vqIdx;
			#pragma omp parallel for
			for( k=0; k<DNNAM[l].inDim; k++ ){
				for( j=0; j<DNNAM[l].vqSize; j++)
					for( z=0; z<DNNAM[l].vqDim; z++ )
						DNNAM[l].VQ[j]+=tmpOutputDNN[l][k*DNNAM[l].vqDim+z]*DNNAM[l].W[j*DNNAM[l].vqDim+z];
				
				// distribute calculated values
				for( j=0; j<DNNAM[l].outDim; j++){
					vqIdx=DNNAM[l].G[j*DNNAM[l].inDim+k]-1; // VQ index is 1-based
					tmpOutputDNN[l+1][j]+=DNNAM[l].VQ[vqIdx];
				}

				for( j=0; j<DNNAM[l].vqSize; j++)
					DNNAM[l].VQ[j]=0.0f;
			}

			#pragma omp parallel for
			for(j=0; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]+=DNNAM[l].B[j];

			if(DEBUG){
				printf("CPU\n");
				for(j=0; j<DNNAM[l].inDim; j++)
					printf("%f ", tmpOutputDNN[l][j]);
				printf("\n");
			}

		}else if( DNNAM[l].type == SOFTMAX ){
			tmp=0;
			for(j=0; j<DNNAM[l].outDim; j++){
				tmpOutputDNN[l+1][j]=exp(tmpOutputDNN[l][j]);
				tmp+=tmpOutputDNN[l+1][j];
			}
			tmp=1.0f/tmp;

			for(j=0; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]*=tmp;

			for(j=0; j<DNNAM[l].outDim; j++)
				if( tmpOutputDNN[l+1][j] < 1e-20 )
					tmpOutputDNN[l+1][j] = 1e-20;
			
		}else if( DNNAM[l].type == PNORM ){
			int pnorm_dim = DNNAM[l].inDim/DNNAM[l].outDim;
			for(j=0; j<DNNAM[l].inDim/pnorm_dim; j++){
				tmp=0;
				for(k=0; k<pnorm_dim; k++)
					tmp+=(tmpOutputDNN[l][j*pnorm_dim+k]*tmpOutputDNN[l][j*pnorm_dim+k]);

				tmpOutputDNN[l+1][j]=sqrt(tmp);
			}
		}else if( DNNAM[l].type == NORMALIZE ){
			tmp=0;
			for(j=0; j<DNNAM[l].outDim; j++)
				tmp+=tmpOutputDNN[l][j]*tmpOutputDNN[l][j];

			tmp/=DNNAM[l].outDim;
			tmp=1.0/sqrt(tmp);
			for(j=0; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]*tmp;

		}else if( DNNAM[l].type == RELU ){
			for(j=0; j<DNNAM[l].outDim; j++)
				(tmpOutputDNN[l][j]>0.0f)?tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]:tmpOutputDNN[l+1][j]=0.0f;

		}else if( DNNAM[l].type == FIXEDSCALE ){
			for(j=0; j<DNNAM[l].outDim; j++)
				tmpOutputDNN[l+1][j]=tmpOutputDNN[l][j]*DNNAM[l].W[j];

		}else if( DNNAM[l].type == SUMGROUP ){
			int idx=0;
			for(j=0; j<DNNAM[l].outDim; j++){
				for(k=0; k<DNNAM[l].G[j]; k++)
					tmpOutputDNN[l+1][j]+=tmpOutputDNN[l][idx+k];
				idx+=DNNAM[l].G[j];
			}
		}

		if( DEBUG && DNNAM[l].type == AFFINE_TRANSFORM && startFrame==0 ){
			SCORETYPE sum=0;
			printf("Layer %d Output\n", l);
			for(i=0; i<DNNAM[l].outDim; i++){
				printf("%e ", tmpOutputDNN[l+1][i]);
				sum+=tmpOutputDNN[l+1][i];
			}
			printf("SUM:%f\n", sum);
		}
		
		
	}
	/* save output of last layer (posterior probability) */
	if( numPrior != DNNAM[numDnnComponent-1].outDim){
		printf("[ERROR] numPrior != outDim_last_layer!\n");
		exit(1);
	}
	for(i=0; i<DNNAM[numDnnComponent-1].outDim; i++){
		if( applyLog ){
			frameScore[startFrame][i]=log(tmpOutputDNN[numDnnComponent][i]);
			if( applyPrior )
				frameScore[startFrame][i]-=Prior[i]; // Prior has been in log already
		}else{
			frameScore[startFrame][i]=tmpOutputDNN[numDnnComponent][i];
		}
	}
}	
#endif


DNN::DNN(){
	tm=NULL;
	p2tm=NULL;
	MMF=NULL;
	p2MMF=NULL;
	DNNAM=NULL;
	frameScore=NULL;

	tmpOutputDNN=NULL;
	fea=NULL;
	buf_fea=NULL;
	
	applyLog=1;
	applyPrior=1;
}
/*
DNN::~DNN(){
	if(p2MMF!=NULL){
		delete [] p2MMF;
		p2MMF=NULL;
	}

	if(MMF!=NULL){
		for(int i=0; i<numTriphone; i++){
			if(MMF[i].tiedStateIndex!=NULL){
				delete [] MMF[i].tiedStateIndex;
				MMF[i].tiedStateIndex=NULL;
			}
		}
		delete [] MMF;
		MMF=NULL;
	}

	if( p2tm != NULL ){
		delete [] p2tm;
		p2tm=NULL;
	}

	if( tm != NULL ){
		for(int i=0; i<numTriphone; i++){
			if(tm[i].transProb!=NULL){
				delete [] tm[i].transProb;
				tm[i].transProb=NULL;
			}
		}
		delete [] tm;
		tm=NULL;
	}

	if( tmpOutputDNN!=NULL ){
		//printf("start free memory\n");
		for(int i=1; i<numDnnComponent+1; i++){
			//printf("free tmpOutput [%d/%d] %p\n", i, numDnnComponent+1, tmpOutputDNN[i]);
			if( tmpOutputDNN[i]!=NULL ){
				delete [] tmpOutputDNN[i];
				tmpOutputDNN[i]=NULL;
			}
		}
		//delete [] tmpOutputDNN;
		util.aligned_free(tmpOutputDNN);
		tmpOutputDNN=NULL;
	}

	if( DNNAM != NULL ){
		for(int i=0; i<numDnnComponent; i++){
			if( DNNAM[i].W!=NULL ){
				delete [] DNNAM[i].W;
				DNNAM[i].W=NULL;
			}
			if( DNNAM[i].B!=NULL ){
				delete [] DNNAM[i].B;
				DNNAM[i].B=NULL;
			}
			if( DNNAM[i].G!=NULL ){
				delete [] DNNAM[i].G;
				DNNAM[i].G=NULL;
			}
			if( DNNAM[i].VQ!=NULL ){
				delete [] DNNAM[i].VQ;
				DNNAM[i].VQ=NULL;
			}
			
		}
		delete [] DNNAM;
		DNNAM=NULL;
	}

	if( Prior != NULL ){
		delete [] Prior;
		Prior=NULL;
	}


	if(frameScore!=NULL){
		for(int i=0; i<MAX_FRAME_NUM; i++){
			delete [] frameScore[i];
			frameScore[i]=NULL;
		}
		delete [] frameScore;
		frameScore=NULL;
	}

	if(fea!=NULL){
		delete [] fea;
		fea=NULL;
	}

	if(buf_fea!=NULL){
		util.aligned_free(buf_fea);
		buf_fea=NULL;
	}
}
*/
SCORETYPE* DNN::readTestFeature(char *fname, int *totFrame, int numDim){
	FILE *fpIn=NULL;
	int rtn;
	int symS=0, symE=0, count=0;
	char tmp[50];
	fea=new SCORETYPE[MAX_FRAME_NUM*numDim];

	if( (fpIn=fopen(fname, "rt"))!=NULL ){
		while( (rtn=fscanf(fpIn, "%s", tmp))== 1 ){
			if( symS==1 ){
				fea[count]=atof(tmp);
				count++;
			}else if( strcmp(tmp, "]")==0 ){
				symE=1;
				break;
			}else if( strcmp(tmp, "[")==0 ){
				symS=1;
				continue;
			}	
		}
	
		*totFrame = (count-1)/numDim;
	}else{
		printf("[ERROR] Cannot read test feature file!\n");
		exit(1);
	}
	return fea;
}


SCORETYPE* DNN::generateTestFeature(int numFrame, int sizeWindow, int numDim, int writeFile){
	FILE *fpOut=NULL;
	int totFrame, halfSizeWindow;

	if( sizeWindow%2 != 1 ){
		printf("[ERROR] 'sizeWindow' in DNN::generateTestFeature() should be an odd number!\n");
		exit(1);
	}
	totFrame=numFrame+sizeWindow-1;
	halfSizeWindow=(sizeWindow-1)>>1;

	srand(time(NULL));
	fea=new SCORETYPE[(totFrame)*numDim];

	for(int j=0;j<numDim; j++)	
		fea[j]=(rand()%RAND_MAX)/(float)RAND_MAX-0.5f;
	for(int i=1; i<=halfSizeWindow; i++)
		for(int j=0;j<numDim; j++)
			fea[i*numDim+j]=fea[j];
	
	for(int i=halfSizeWindow+1; i<halfSizeWindow+numFrame; i++)
		for(int j=0;j<numDim; j++)
			fea[i*numDim+j]=(rand()%RAND_MAX)/(float)RAND_MAX-0.5f;

	for(int i=halfSizeWindow+numFrame; i<totFrame; i++)
		for(int j=0;j<numDim; j++)
			fea[i*numDim+j]=fea[(halfSizeWindow+numFrame-1)*numDim+j];

	if( writeFile != 0 ){
		if( (fpOut=fopen("testfeat.ark", "wt"))!= NULL ){
			fprintf(fpOut, "test [ ");
			for(int i=0; i<totFrame; i++){
				for(int j=0; j<numDim; j++)
					fprintf(fpOut, "%f ", fea[i*numDim+j]);
				fprintf(fpOut, "\n");
			}
			fprintf(fpOut, "]\n");
			fclose(fpOut);
		}else{
			printf("[ERROR] Cannot write test features to file!\n");
			exit(1);
		}
		
	}

	return fea;
}

