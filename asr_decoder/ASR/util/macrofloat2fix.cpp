#include "recog.h"

// for legacy ASR AM

#define SCALE_MIXWEIGHT	1024
#define SCALE_MEAN	4096
#define SCALE_VARIANCE	64 //, reciprocal*SCALE_VARIANCE, v -> (1/v), (1/v)*64
#define SCALE_GCONST	1024


// for new ASR AM
/*
#define SCALE_MIXWEIGHT	4096	//1024
#define SCALE_MEAN	1024	//4096
#define SCALE_VARIANCE	256	// 64, reciprocal*SCALE_VARIANCE, v -> (1/v), (1/v)*64
#define SCALE_GCONST	256	//1024
*/

void float2fix(float *dataVector, int *passOn, int scale, int allocateSize){
	memset(passOn, 0, sizeof(int)*allocateSize);
	for(int j=0;j<allocateSize;j++){
		if(dataVector[j]!=0)
			passOn[j]=(int)floor(dataVector[j]*scale+0.5);
	}
}

int main(int argc, char **argv){
	// ==== For saving memory space, transition probs are no longer used. ====
	if(argc!=3){
		printf("Usage: %s macro.float.bin macro.fix.bin\n", argv[0]);
		printf("Transition Probs are no longer used.\n");
		printf("Scale on mixWeight -> %d\n", SCALE_MIXWEIGHT);
		printf("Scale on mean -> %d\n", SCALE_MEAN);
		printf("Scale on (1/variance) -> %d\n", SCALE_VARIANCE);
		printf("Scale on gconst -> %d\n", SCALE_GCONST);
		exit(1);
	}
	int *passOn;

	// readmacro
	MACRO macro;
	macro.readMacroBin(argv[1]);
	macro.tmNum=0; // transition matrix is not needed

	// writemacro
	UTILITY util;
	FILE *out=util.openFile(argv[2],"wb");
	// write global info. It has been obtained in allocateMem()
	fwrite(&macro.modelNum, sizeof(int), 1, out); 
	fwrite(&macro.streamWidth, sizeof(int), 1, out); 
	fwrite(&macro.tsNum, sizeof(int), 1, out);
	fwrite(&macro.tmNum, sizeof(int), 1, out);
	// write model
	for (int i=0;i<macro.modelNum;i++){
		fwrite(macro.p2MMF[i]->str, sizeof(char)*MODELSTRLEN, 1, out);
		fwrite(&macro.p2MMF[i]->stateNum, sizeof(int), 1, out);
		fwrite(macro.p2MMF[i]->tiedStateIndex, sizeof(int), macro.p2MMF[i]->stateNum-2, out);
		fwrite(&macro.p2MMF[i]->tiedMatrixIndex, sizeof(int),1,out);
	}
	// write tied state
	int allocateSize;
	for(int i=0;i<macro.tsNum;i++){
		fwrite(macro.p2ts[i]->str, sizeof(char)*MODELSTRLEN, 1, out);
		fwrite(&macro.p2ts[i]->mixtureNum, sizeof(int), 1, out);
		// mixture weight
		allocateSize=macro.p2ts[i]->mixtureNum;
		passOn = new int[allocateSize];		
		for(int j=0;j<allocateSize;j++)
			passOn[j]=(int)(macro.p2ts[i]->mixtureWeight[j]*SCALE_MIXWEIGHT);
		fwrite(passOn, sizeof(int), allocateSize, out);
		delete [] passOn;
		// mean
		allocateSize=macro.p2ts[i]->mixtureNum*macro.streamWidth;
		passOn = (int*) util.aligned_malloc(allocateSize*sizeof(int), ALIGNSIZE);
		float2fix(macro.p2ts[i]->mean, passOn, SCALE_MEAN, allocateSize);
		fwrite(passOn, sizeof(int), allocateSize, out);
		util.aligned_free(passOn);
		// variance
		allocateSize=macro.p2ts[i]->mixtureNum*macro.streamWidth;
		passOn = (int*) util.aligned_malloc(allocateSize*sizeof(int), ALIGNSIZE);		
		float2fix(macro.p2ts[i]->variance, passOn, SCALE_VARIANCE, allocateSize);
		fwrite(passOn, sizeof(int), allocateSize, out);
		util.aligned_free(passOn);
		// gconst
		allocateSize=macro.p2ts[i]->mixtureNum;
		passOn = new int[allocateSize];
		for(int j=0;j<allocateSize;j++)
			passOn[j]=(int)(macro.p2ts[i]->gconst[j]*SCALE_GCONST);
		fwrite(passOn, sizeof(int), allocateSize, out);
		delete [] passOn;
	}
	fclose(out);

	return 0;
}
