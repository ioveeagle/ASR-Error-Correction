#include "recog.h"

void MACRO::readMacroBin(char *fname) {
	size_t st;
	allocateMem(fname);

	UTILITY util;
	FILE *in=util.openFile(fname,"rb");
	// read global info. It has been obtained in allocateMem()
	st=fread(&modelNum, sizeof(int), 1, in); 
	st=fread(&streamWidth, sizeof(int), 1, in); 
	st=fread(&tsNum, sizeof(int), 1, in);  // # of tied-state
	st=fread(&tmNum, sizeof(int), 1, in);  // # of tied-matrix (transition matrix)
	// read model
	for (int i=0;i<modelNum;i++){
		st=fread(MMF[i].str, sizeof(char)*MODELSTRLEN, 1, in);
		st=fread(&MMF[i].stateNum, sizeof(int), 1, in);
		MMF[i].tiedStateIndex = new int[MMF[i].stateNum-2];
		st=fread(MMF[i].tiedStateIndex, sizeof(int), MMF[i].stateNum-2, in);
		st=fread(&MMF[i].tiedMatrixIndex, sizeof(int),1,in);
	}	
	// read tied state
	for(int i=0;i<tsNum;i++){
		st=fread(ts[i].str, sizeof(char)*MODELSTRLEN, 1, in);
		st=fread(&ts[i].mixtureNum, sizeof(int), 1, in);
		ts[i].mixtureWeight = (SCORETYPE*) util.aligned_malloc(ts[i].mixtureNum*sizeof(SCORETYPE), ALIGNSIZE);
		ts[i].mean = (SCORETYPE*) util.aligned_malloc(ts[i].mixtureNum*streamWidth*sizeof(SCORETYPE), ALIGNSIZE);
		ts[i].variance = (SCORETYPE*) util.aligned_malloc(ts[i].mixtureNum*streamWidth*sizeof(SCORETYPE), ALIGNSIZE);
		ts[i].gconst = (SCORETYPE*) util.aligned_malloc(ts[i].mixtureNum*sizeof(SCORETYPE), ALIGNSIZE);
		
		st=fread(ts[i].mixtureWeight, sizeof(SCORETYPE), ts[i].mixtureNum, in);
		st=fread(ts[i].mean, sizeof(SCORETYPE), ts[i].mixtureNum*streamWidth, in);
		st=fread(ts[i].variance, sizeof(SCORETYPE), ts[i].mixtureNum*streamWidth, in);
		st=fread(ts[i].gconst, sizeof(SCORETYPE), ts[i].mixtureNum, in);
	}
	// tied matrix data
	for(int i=0;i<tmNum;i++){
		st=fread(tm[i].str, sizeof(char)*MODELSTRLEN, 1, in);
		st=fread(&tm[i].stateNum, sizeof(int), 1, in);
		tm[i].transProb = new float[tm[i].stateNum*tm[i].stateNum];
		st=fread(tm[i].transProb, sizeof(float), tm[i].stateNum*tm[i].stateNum, in);
	}
	fclose(in);
    	
  // pointers for fast access
	for(int i=0;i<modelNum;i++)
		p2MMF[i]=MMF+i;
	for(int i=0;i<tsNum;i++)
		p2ts[i]=ts+i;
	for(int i=0;i<tmNum;i++)
		p2tm[i]=tm+i;
}

void MACRO::allocateMem(char *fname){
	size_t st;

	UTILITY util;
	FILE *in=util.openFile(fname,"rb");
	st=fread(&modelNum, sizeof(int), 1, in); 
	st=fread(&streamWidth, sizeof(int), 1, in);
	st=fread(&tsNum, sizeof(int), 1, in);
	st=fread(&tmNum, sizeof(int), 1, in);
	fclose(in);

	ts = new TIEDSTATEVEC[tsNum];
	p2ts = new TIEDSTATEVEC * [tsNum];
	tm = new TIEDMATRIX[tmNum];
	p2tm = new TIEDMATRIX * [tmNum];
	MMF = new MASTERMACROFILE[modelNum];
	p2MMF = new MASTERMACROFILE * [modelNum];
}

MACRO::MACRO(){
	ts = NULL;
	p2ts = NULL;
	tm = NULL;
	p2tm = NULL;
	MMF = NULL;
	MMF = NULL;
	p2MMF = NULL;
}

MACRO::~MACRO(){
	UTILITY util;
	if(MMF!=NULL){
		for(int i=0;i<modelNum;i++){
			if(MMF[i].tiedStateIndex!=NULL)
				delete [] MMF[i].tiedStateIndex;
		}
		delete [] MMF;
		MMF=NULL;
	}
	if(ts!=NULL){
		for(int i=0;i<tsNum;i++){
			util.aligned_free(ts[i].mixtureWeight);
			util.aligned_free(ts[i].mean);
			util.aligned_free(ts[i].variance);
			util.aligned_free(ts[i].gconst);
		}
		delete [] ts;
		ts=NULL;
	}
	if(p2ts!=NULL){
		delete [] p2ts;
		p2ts=NULL;
	}
	if(tm!=NULL){
		for(int i=0;i<tmNum;i++){
			if(tm[i].transProb!=NULL)
				delete [] tm[i].transProb;
		}
		delete [] tm;
		tm=NULL;
	}
	if(p2tm!=NULL){
		delete [] p2tm;
		p2tm=NULL;
	}
	if(p2MMF!=NULL){
		delete [] p2MMF;
		p2MMF=NULL;
	}
}
